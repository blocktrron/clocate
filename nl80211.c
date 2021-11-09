#include <sys/socket.h>

#include <linux/nl80211.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <errno.h>
#include <net/if.h>

#include "nl80211.h"

#define RESULT_DONE	0x1
#define RESULT_ABORTED	0x2

static struct nla_policy scan_result_policy[NUM_NL80211_ATTR] = {
	[NL80211_BSS_TSF] = { .type = NLA_U64 },
	[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
	[NL80211_BSS_BSSID] = { },
	[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
	[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
	[NL80211_BSS_INFORMATION_ELEMENTS] = { },
	[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
	[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
	[NL80211_BSS_STATUS] = { .type = NLA_U32 },
	[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
	[NL80211_BSS_BEACON_IES] = { },
};

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg) {
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}


static int finish_handler(struct nl_msg *msg, void *arg) {
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int callback_trigger(struct nl_msg *msg, void *arg) {
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	int *results = arg;

	switch (gnlh->cmd) {
		case NL80211_CMD_SCAN_ABORTED:
			*results |= RESULT_ABORTED;
		case NL80211_CMD_NEW_SCAN_RESULTS:
			*results |= RESULT_DONE;
		default:
			break;
	}

	return NL_SKIP;
}

static int no_seq_check(struct nl_msg *msg, void *arg) {
	return NL_OK;
}

static int do_scan_trigger(struct nl_sock *socket, int if_index, int driver_id) {
	struct nl_msg *msg;
	struct nl_cb *cb;
	struct nl_msg *ssids_to_scan;
	int err = 0;
	int ret;
	int mcid;
	int result = 0;

	mcid = genl_ctrl_resolve_grp(socket, "nl80211", "scan");
	if (mcid < 0) {
		ret = -ENOSYS;
		goto error;
	}

	nl_socket_add_membership(socket, mcid);

	msg = nlmsg_alloc();
	if (!msg) {
		ret = -ENOMEM;
		goto error;
	}

	ssids_to_scan = nlmsg_alloc();
	if (!ssids_to_scan) {
		ret = -ENOMEM;
		goto error;
	}

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		ret = -ENOMEM;
		goto error_free;
	}

	/* Message */
	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, driver_id,
		    0, 0, NL80211_CMD_TRIGGER_SCAN, 0);
	nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);
	nla_put(ssids_to_scan, 1, 0, "");
	nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids_to_scan);

	/* Callbacks */
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_trigger, &result);  // Add the callback.
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);  // No sequence checking for multicast messages.	nl_cb_set(cb, NL_CB_MSG_OUT, NL_CB_DEBUG, NULL, NULL);

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);

	ret = nl_send_auto(socket, msg);

	while (!(result & RESULT_DONE) && err == 0)
		nl_recvmsgs(socket, cb);
	
	if (err) {
		ret = err;
		goto error_free;
	}

	if (result & RESULT_ABORTED)
		ret = -ECANCELED;

error_free:
	nl_socket_drop_membership(socket, mcid);

	if (msg)
		nlmsg_free(msg);

	if (cb)
		nl_cb_put(cb);
	if (ssids_to_scan)
		nlmsg_free(ssids_to_scan);
error:
	if (ret < 0) {
		fprintf(stderr, "%s: %d", __func__, ret);
		return ret;
	}

	return 0;
}

#define NEXT_IE_OFFSET(ie_buf)	(ie_buf + (uint8_t)ie_buf[1] + 2)
static char *find_ie(char *ies, int ies_len, char tag)
{
	char *current_ie = ies;

	while (current_ie < ies + ies_len && NEXT_IE_OFFSET(current_ie) <= ies + ies_len) {
		if (current_ie[0] == tag)
			return current_ie;
			
		current_ie = NEXT_IE_OFFSET(current_ie);
	}

	return NULL;
}

static int result_cb(struct nl_msg *msg, void *arg) {
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NUM_NL80211_ATTR];
	struct nlattr *bss[NUM_NL80211_ATTR];

	struct scan_results *results;
	struct scan_result *result;

	results = arg;
	results->result_count++;
	results->results = realloc(results->results, results->result_count * sizeof(struct scan_result));
	if (results->results < 0)
		return -ENOMEM;

	result = &results->results[results->result_count - 1];
	memset(result, 0, sizeof(result));

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
	if (!tb[NL80211_ATTR_BSS] ||
	    nla_parse_nested(bss, NL80211_BSS_MAX,tb[NL80211_ATTR_BSS], scan_result_policy))
		return NL_SKIP;

	if (bss[NL80211_BSS_SIGNAL_MBM])
		result->signal = ((int)nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM])) / 100;

	if (bss[NL80211_BSS_BSSID])
		memcpy(result->bssid, nla_data(bss[NL80211_BSS_BSSID]), sizeof(char) * 6);

	if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
		char *ies = nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
		int ie_size = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);

		char *ssid_ie = find_ie(ies, ie_size, 0);

		if (ssid_ie && ssid_ie[1] < 33 && &ssid_ie[1] + ssid_ie[1] < ies + ie_size)
			memcpy(result->ssid, &ssid_ie[2], ssid_ie[1]);
	}

	return NL_SKIP;
}

static int get_scan_results(struct nl_sock *socket, int if_index, int driver_id, struct scan_results *results)
{
	struct nl_msg *msg;
	int ret;

	msg = nlmsg_alloc();
	genlmsg_put(msg, 0, 0, driver_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
	nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);

	nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, result_cb, results);
	
	ret = nl_send_auto(socket, msg);
	if (ret < 0)
		goto error;

	ret = nl_recvmsgs_default(socket);
	if (ret < 0)
		goto error;

error:
	nlmsg_free(msg);
	if (ret)
		fprintf(stderr, "%s: %d", __func__, ret);
	return ret;
}

int perform_scan( struct scan_results *results, const char *ifname)
{
	struct nl_sock *socket = nl_socket_alloc();
	int ifindex = if_nametoindex(ifname);
	int driver_id;
	int ret;

	genl_connect(socket);

	driver_id = genl_ctrl_resolve(socket, "nl80211");
	if (driver_id < 0) {
		ret = -ENOSYS;
		goto error;
	}

	ret = do_scan_trigger(socket, ifindex, driver_id);
	if (ret)
		goto error;

	ret = get_scan_results(socket, ifindex, driver_id, results);
	if (ret)
		goto error;

error:
	if (ret)
		fprintf(stderr, "%s: %d", __func__, ret);

	nl_socket_free(socket);
	return ret;
}

static int iface_list_result_cb(struct nl_msg *msg, void *arg) {
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NUM_NL80211_ATTR];
	struct nlattr *bss[NUM_NL80211_ATTR];
	struct clocate_interfaces *results = arg;
	char *outbuf;
	char *buf;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
	if (!tb[NL80211_ATTR_IFNAME])
		return NL_SKIP;

	results->count++;
	results->buf = realloc(results->buf, results->count * (IF_NAMESIZE * sizeof(char)));
	if (results->buf < 0)
		return -ENOMEM;

	outbuf = &results->buf[(results->count - 1) * IF_NAMESIZE];
	memset(outbuf, 0, IF_NAMESIZE * sizeof(char));


	buf = nla_get_string(tb[NL80211_ATTR_IFNAME]);
	memcpy(outbuf, buf, strnlen(buf, IF_NAMESIZE) * sizeof(char));

	return NL_SKIP;
}

int get_wireless_interfaces(struct clocate_interfaces *results)
{
	struct nl_sock *socket = nl_socket_alloc();
	int driver_id;
	int ret;

	genl_connect(socket);

	driver_id = genl_ctrl_resolve(socket, "nl80211");
	if (driver_id < 0) {
		ret = -ENOSYS;
		goto error;
	}

	struct nl_msg *msg;

	msg = nlmsg_alloc();
	genlmsg_put(msg, 0, 0, driver_id, 0, NLM_F_DUMP, NL80211_CMD_GET_INTERFACE, 0);

	nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, iface_list_result_cb, results);

	ret = nl_send_auto(socket, msg);
	if (ret < 0)
		goto error;

	ret = nl_recvmsgs_default(socket);
	if (ret < 0)
		goto error;

error:
	if (msg)
		nlmsg_free(msg);

	if (ret < 0) {
		fprintf(stderr, "%s: %d", __func__, ret);
		return ret;
	}

	nl_socket_free(socket);

	return 0;
}

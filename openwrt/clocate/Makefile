include $(TOPDIR)/rules.mk

PKG_NAME:=clocate
PKG_VERSION:=$(shell git show -s --format=%cd --date=short)
PKG_RELEASE:=1

PKG_FILE_DEPENDS:=$(CURDIR)/../..

include $(INCLUDE_DIR)/package.mk

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	ln -s $(CURDIR)/../../.git $(PKG_BUILD_DIR)/.git
	cd $(PKG_BUILD_DIR) && git checkout .
endef

define Package/clocate
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+libcurl +libnl-tiny +libjson-c +libubox +libubus +libblobmsg-json
  TITLE:=Location
endef

define Package/clocate/install
	$(INSTALL_DIR) $(1)/sbin $(1)/etc/config $(1)/etc/init.d

	$(CP) ./files/clocate.config $(1)/etc/config/clocate
	$(INSTALL_BIN) ./files/clocate.init $(1)/etc/init.d/clocate

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/clocate $(1)/sbin/clocate
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/clocated $(1)/sbin/clocated
endef

MAKE_FLAGS += LIBNL_TINY=1 DAEMON=1

$(eval $(call BuildPackage,clocate))

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
  DEPENDS:=+libcurl +libnl-tiny +libjson-c
  TITLE:=Location
endef

define Package/clocate/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/clocate $(1)/sbin/clocate
endef

MAKE_FLAGS += LIBNL_TINY=1

$(eval $(call BuildPackage,clocate))

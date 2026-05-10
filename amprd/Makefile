include $(TOPDIR)/rules.mk

PKG_NAME:=amprd
PKG_VERSION:=2.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://www.yo2loj.ro/hamprojects

PKG_HASH:=skip
DOWNLOAD_NO_CHECK:=1

PKG_LICENSE:=GPL-2.0-only
PKG_MAINTAINER:=Bill Hill <bugs@wbh.org>

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/amprd
  SECTION:=net
  CATEGORY:=Network
  TITLE:=IPIP encapsulation daemon for AMPR network
endef

define Package/amprd/description
 A user-space IPIP encapsulation daemon with automatic
 RIPv2 multicast processing and multiple tunnel support
 for the AMPR network.
endef

define Package/amprd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) \
		$(PKG_BUILD_DIR)/amprd \
		$(1)/usr/sbin/
endef

# musl libc compatibility: UDP fields have different names
PKG_BUILD_FLAGS += no-mips16
PKG_BUILD_PARALLEL := 1

# musl libc compatibility: UDP fields have different names
PKG_BUILD_FLAGS += no-mips16
PKG_BUILD_PARALLEL := 1

$(eval $(call BuildPackage,amprd))

include $(TOPDIR)/rules.mk

PKG_NAME:=ampr-ripd
PKG_VERSION:=2.3
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://www.yo2loj.ro/hamprojects

PKG_HASH:=skip

PKG_LICENSE:=GPL-2.0-only
PKG_MAINTAINER:=Bill Hill <bugs@wbh.org>

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/ampr-ripd
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Routing daemon for the AMPR network
endef

define Package/ampr-ripd/description
 Routing daemon written in C similar to Hessu's rip44d
 including optional resending of RIPv2 broadcasts
 for router injection.
endef

define Package/ampr-ripd/install
	$(INSTALL_DIR) $(1)/usr/sbin

	$(INSTALL_BIN) \
		$(PKG_BUILD_DIR)/ampr-ripd \
		$(1)/usr/sbin/
endef

$(eval $(call BuildPackage,ampr-ripd))

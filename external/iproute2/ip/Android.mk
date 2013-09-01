LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := ip.c ipaddress.c ipaddrlabel.c iproute.c iprule.c   \
	rtm_map.c iptunnel.c ip6tunnel.c tunnel.c ipneigh.c ipntable.c \
	iplink.c ipmaddr.c ipmonitor.c ipmroute.c ipprefix.c ipxfrm.c  \
	xfrm_state.c xfrm_policy.c xfrm_monitor.c iplink_vlan.c        \
	link_veth.c link_gre.c iplink_can.c iptuntap.c

LOCAL_MODULE := ip

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libc libm libdl

LOCAL_SHARED_LIBRARIES += libiprouteutil libnetlink

LOCAL_C_INCLUDES := $(KERNEL_HEADERS) external/iproute2/include

LOCAL_CFLAGS := -O2 -g -W -Wall 

LOCAL_LDFLAGS := -Wl,-export-dynamic -Wl,--no-gc-sections

include $(BUILD_EXECUTABLE)


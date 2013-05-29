#ifndef _WIRELESS_SNMP
#define _WIRELESS_SNMP

#if WIRELESS_EXT <= 11
#ifndef SIOCDEVPRIVATE
#define SIOCDEVPRIVATE              0x8BE0
#endif
#define SIOCIWFIRSTPRIV             SIOCDEVPRIVATE
#endif

#ifndef AP_MODE
#define RT_PRIV_IOCTL               (SIOCIWFIRSTPRIV + 0x0E)
#else
#define RT_PRIV_IOCTL               (SIOCIWFIRSTPRIV + 0x01)
#endif  /* AP_MODE */

#define RTPRIV_IOCTL_SET            (SIOCIWFIRSTPRIV + 0x02)

#ifdef DBG
#define RTPRIV_IOCTL_BBP            (SIOCIWFIRSTPRIV + 0x03)
#define RTPRIV_IOCTL_MAC            (SIOCIWFIRSTPRIV + 0x05)
#define RTPRIV_IOCTL_E2P            (SIOCIWFIRSTPRIV + 0x07)
#endif

#define RTPRIV_IOCTL_STATISTICS         (SIOCIWFIRSTPRIV + 0x09)
#define RTPRIV_IOCTL_ADD_PMKID_CACHE    (SIOCIWFIRSTPRIV + 0x0A)
#define RTPRIV_IOCTL_RADIUS_DATA        (SIOCIWFIRSTPRIV + 0x0C)
#define RTPRIV_IOCTL_GSITESURVEY        (SIOCIWFIRSTPRIV + 0x0D)
#define RTPRIV_IOCTL_ADD_WPA_KEY        (SIOCIWFIRSTPRIV + 0x0E)
#define RTPRIV_IOCTL_GET_MAC_TABLE      (SIOCIWFIRSTPRIV + 0x0F)
#define RTPRIV_IOCTL_STATIC_WEP_COPY    (SIOCIWFIRSTPRIV + 0x10)
#define RTPRIV_IOCTL_WSC_PROFILE        (SIOCIWFIRSTPRIV + 0x12)
#define RT_QUERY_ATE_TXDONE_COUNT       0x0401
#define OID_GET_SET_TOGGLE              0x8000

CVoidType wirelessInit(void);

#endif	/* _WIRELESS_SNMP */

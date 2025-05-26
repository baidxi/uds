#include <sys/socket.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <linux/netlink.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <linux/can/netlink.h>
#include <linux/can/raw.h>
#include <net/if.h>

#include <cstring>

#include "can.hpp"

struct set_req {
    struct nlmsghdr hdr;
    struct ifinfomsg ifi;
    char playload[1024];
};

struct netlink_req {
    struct nlmsghdr hdr;
    struct ifinfomsg ifi;
};

#define NLMSG_TAIL(nmsg) ((struct rtattr*)(((char *)(nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

can_hw::can_hw()
{
    netlink_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (netlink_fd < 0) {
        fprintf(stderr, "netlink socket err\n");
    }
}

can_hw::~can_hw()
{
    if (can_sock_fd > 0)
        close(can_sock_fd);
    if (netlink_fd > 0)
        close(netlink_fd);
}

int can_hw::send(const void *buf, size_t len)
{
   return ::send(can_sock_fd, buf, len, 0);
}

size_t can_hw::recv(void *buf, size_t size)
{
    return ::recv(can_sock_fd, buf, size, 0);
}

int addattr_l(struct nlmsghdr *n, unsigned int maxlen, int type, const void *data, int alen)
{
    unsigned int len = RTA_LENGTH(alen);
    struct rtattr *rta;

    if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > maxlen)
    {
        fprintf(stderr, "addattr_l ERR: message exceeded bound of %d\n", maxlen);
        return -1;
    }

    rta = NLMSG_TAIL(n);
    rta->rta_type = type;
    rta->rta_len = len;
    if (alen)
        ::memcpy(RTA_DATA(rta), data, alen);
    
    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);

    return 0;
}

struct rtattr *addattr_nest(struct nlmsghdr *n, int maxlen, int type)
{
    struct rtattr *nest = NLMSG_TAIL(n);

    addattr_l(n, maxlen, type, nullptr, 0);

    return nest;
}

int addattr_nest_end(struct nlmsghdr *n, struct rtattr *nest)
{
    nest->rta_len = (char *)NLMSG_TAIL(n) - (char *)nest;
    return n->nlmsg_len;
}

int addattr32(struct nlmsghdr *n, int maxlen, int type, uint32_t data)
{
    return addattr_l(n, maxlen, type, &data, sizeof(uint32_t));
}

int can_hw::init(const std::string &ifname, uint32_t bitrate)
{
    int ret;
    can_bittiming bt{.bitrate = bitrate};
    rtattr *linkinfo = nullptr, *data = nullptr;
    const char *type = "can";
    set_req req{};
    int ifindex = -1;

    req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(ifinfomsg));
    req.hdr.nlmsg_flags = NLM_F_REQUEST;
    req.hdr.nlmsg_type = RTM_NEWLINK;
    req.ifi.ifi_family = AF_UNSPEC;

    ifindex = if_nametoindex(ifname.c_str());
    if (ifindex == 0) {
        fprintf(stderr, "no dev %s\n", ifname.c_str());
        return -1;
    }

    addattr32(&req.hdr, sizeof(req), IFLA_LINK, ifindex);
    req.ifi.ifi_index = ifindex;
    linkinfo = addattr_nest(&req.hdr, sizeof(req), IFLA_LINKINFO);
    addattr_l(&req.hdr, sizeof(req), IFLA_INFO_KIND, (void *)type, strlen(type));
    data = addattr_nest(&req.hdr, sizeof(req), IFLA_INFO_DATA);
    addattr_l(&req.hdr, 1024, IFLA_CAN_BITTIMING, &bt, sizeof(bt));
    addattr_nest_end(&req.hdr, data);
    addattr_nest_end(&req.hdr, linkinfo);

    iovec iov = {
        .iov_base = &req.hdr,
        .iov_len = req.hdr.nlmsg_len,
    };

    sockaddr_nl nladdr = {
        .nl_family = AF_NETLINK,
    };

    msghdr msg = {
        .msg_name = &nladdr,
        .msg_namelen = sizeof(nladdr),
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    req.hdr.nlmsg_flags |= NLM_F_ACK;
    req.hdr.nlmsg_seq = 1;

    ret = sendmsg(netlink_fd, &msg, 0);

    if (ret < 0) {
        fprintf(stderr, "sendmsg FAIL\n");
        return -1;
    }

    return 0;
}

static void fill_msg(struct netlink_req *req, const std::string &ifname)
{
    req->hdr.nlmsg_len = NLMSG_LENGTH(sizeof(req->ifi));
    req->hdr.nlmsg_type = RTM_NEWLINK;
    req->hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    req->ifi.ifi_family = AF_UNSPEC;
    req->ifi.ifi_index = if_nametoindex(ifname.c_str());
}

int can_hw::ifchange(const std::string &ifname, int state)
{
    struct netlink_req req{};
    struct sockaddr_nl kernel_sa{};
    int ret;

    fill_msg(&req, ifname);

    req.ifi.ifi_change = IFF_UP;
    if (state)
        req.ifi.ifi_flags |= IFF_UP;
    else
        req.ifi.ifi_flags &= ~IFF_UP;

    kernel_sa.nl_family = AF_NETLINK;

    iovec iov = {
        &req.hdr,
        req.hdr.nlmsg_len,
    };

    msghdr msg = {
        &kernel_sa,
        sizeof(kernel_sa),
        &iov,
        1,
        nullptr,
        0, 0
    };

    return sendmsg(netlink_fd, &msg, 0);
}

int can_hw::up(const std::string &ifname)
{
    return ifchange(ifname, 1);
}

int can_hw::down(const std::string &ifname)
{
    return ifchange(ifname, 0);
}

int can_hw::bind(const std::string &ifname)
{
    sockaddr_can addr;
    ifreq ifr;

    can_sock_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_sock_fd < 0)
        fprintf(stderr, "can socket err\n");

    strcpy(ifr.ifr_name, ifname.c_str());
    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex(ifname.c_str());

    return ::bind(can_sock_fd, (struct sockaddr *)&addr, sizeof(addr));
}

int can_hw::bind(int fd, const std::string &ifname)
{
    sockaddr_can addr;
    ifreq ifr;

    can_sock_fd = fd;
    
    strcpy(ifr.ifr_name, ifname.c_str());
    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex(ifname.c_str());

    return ::bind(can_sock_fd, (struct sockaddr *)&addr, sizeof(addr));
}
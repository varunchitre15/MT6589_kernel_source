#!/system/bin/sh
# traffic control; arg1:ifname, arg2: rx, arg 3 tx.
#
export PATH=/data:$PATH

# clear rules
tc qdisc del dev $1 root
tc qdisc del dev $1 ingress
tc qdisc del dev ifb0 root

# set interface throttle
tc qdisc add dev $1 root handle 1: htb default 1 r2q 1000
tc class add dev $1 parent 1: classid 1:1 htb rate $3kbit
ifconfig ifb0 up
tc qdisc add dev ifb0 root handle 1: htb default 1 r2q 1000
tc class add dev ifb0 parent 1: classid 1:1 htb rate $2kbit
tc qdisc add dev $1 ingress
tc filter add dev $1 parent ffff: protocol ip prio 10 u32 match \
            u32 0 0 flowid 1:1 action mirred egress redirect dev ifb0

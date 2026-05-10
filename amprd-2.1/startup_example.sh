#
# Example script to start amprd 1.0 - Marius, YO2LOJ,  <marius@yo2loj.ro>
#
#
#
# This is needed so your ampr gateway will also be reacheable from the internet
#
#
# The idea behind:
#   - the default route for the table 'default' is the tunnel device
#   - all incoming traffic from the tunnel with the ip 44.182.21.1 (in my case)
#     will be handled by table 'default'
#   - except traffic coming from the tunnel with the ip 44.182.21.1 with 
#     destination 44.0.0.0/8 which will be handled by table 'main'
#
# The default route for table 'default' has to be created on each daemon launch
# since it gets lost on interface shutdown.
#
# The rules are persistent and cummulative so there is a check if they exist
# so they are not doubled


echo -n "(Re)starting ampr tunnels... "

TST=`ps -A | grep amprd`

if [ "$TST" != "" ]; then
    killall amprd
fi

/usr/sbin/amprd
ip route add default dev ampr0 table default


RTST=`ip rule | grep 44.182.21.1`

if [ "$RTST" == "" ]; then
    ip rule add from 44.182.21.1 table default
    ip rule add from 44.182.21.1 to 44.0.0.0/8 table main
fi

echo "Done."


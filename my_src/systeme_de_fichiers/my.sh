# status led = 10
echo 10 > /sys/class/gpio/export
# power led = 362
echo 362 > /sys/class/gpio/export
# set to output
echo out > /sys/class/gpio/gpio10/direction
echo out > /sys/class/gpio/gpio362/direction

# pour les bouttons 
#    88:    31   0   0   0  sunxi_pio_edge   0 Edge   irq_k1
#    90:    42   0   0   0  sunxi_pio_edge   2 Edge   irq_k2
#    91:    36   0   0   0  sunxi_pio_edge   3 Edge   irq_k3
echo 0 > /sys/class/gpio/export
echo 2 > /sys/class/gpio/export
echo 3 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio0/direction
echo in > /sys/class/gpio/gpio2/direction
echo in > /sys/class/gpio/gpio3/direction
cat /sys/class/gpio/gpio0/value
cat /sys/class/gpio/gpio2/value
cat /sys/class/gpio/gpio3/value

#set to rising edge
echo rising > /sys/class/gpio/gpio0/edge
echo rising > /sys/class/gpio/gpio2/edge
echo rising > /sys/class/gpio/gpio3/edge
# #set to falling edge
# echo falling > /sys/class/gpio/gpio0/edge
# echo falling > /sys/class/gpio/gpio2/edge
# #set to both edge
# echo both > /sys/class/gpio/gpio3/edge



# unexport all
# echo 10 > /sys/class/gpio/unexport
# echo 362 > /sys/class/gpio/unexport
# echo 0 > /sys/class/gpio/unexport
# echo 2 > /sys/class/gpio/unexport
# echo 3 > /sys/class/gpio/unexport




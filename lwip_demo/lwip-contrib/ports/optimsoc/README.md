# LwIP Port for OpTiMSoC
This is the lwip port for OpTiMSoC. It is the software driver to access hardware 
functions.

The lwipopts.h file define user specific specifications. It enables or disables
lwip options and uses default values from lwip/src/opt.h

The file "sys_arch.c" imports the function "or1k_timer_get_ticks" from 
or1k to the functions "sys_jiffies" and "sys_now".


## IMPORTANT REMARK:
To ensure the functionality of the lwip stack, sys_now returns the current time 
in milliseconds. This is only possible if the timer of or1k has a tick frequency
of 1ms.

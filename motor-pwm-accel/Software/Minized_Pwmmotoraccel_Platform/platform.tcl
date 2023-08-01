# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\xilinx\motor-pwm-accel\Software\Minized_Pwmmotoraccel_Platform\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\xilinx\motor-pwm-accel\Software\Minized_Pwmmotoraccel_Platform\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {Minized_Pwmmotoraccel_Platform}\
-hw {C:\xilinx\motor-pwm-accel\Z_system_wrapper.xsa}\
-proc {ps7_cortexa9_0} -os {standalone} -out {C:/xilinx/motor-pwm-accel/Software}

platform write
platform generate -domains 
platform active {Minized_Pwmmotoraccel_Platform}
platform generate
bsp reload
bsp reload
platform generate
platform clean
platform generate
platform active {Minized_Pwmmotoraccel_Platform}
platform generate -domains 
platform clean
platform generate
platform generate

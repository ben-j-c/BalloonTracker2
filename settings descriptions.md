debug : Enables debug print statements
com_port : Integer for the com port to use
com_timeout : The time to wait for the com to connect
com_baud : The baud rate for the com port; must mach arduino
camera : The url for the RSTP stream
socket_port : The port to use when connecting to the GUI; must match GUI
sensor_width : Width (in mm) of the camera sensor
sensor_height : Height (in mm) of the camera sensor
focal_length_min : Calibrated focal length at no zoom
focal_length_max : Calibrated focal length at max zoom
principal_x : Calibrated principal point
principal_y : Calibrated principal point
image_resize_factor : Factor for resizing input image for image processing
thresh_red : 8-bit integer for used for thresholding the red chanel (greater than)
thresh_s : 8-bit integer for thresholding pixel intensity (greater than)
thresh_green : 8-bit integer for thresholding the green chanel (less than)
motor_pan_factor : Conversion factor from degrees to microseconds on pan motor
motor_pan_min : Minimum safe pan
motor_pan_max : Maximum safe pan
motor_pan_forward : When pan motor is set to this value it looks forward
motor_tilt_factor : Conversion factor from degrees to microseconds on the tilt motor
motor_tilt_min : Minimum safe tilt
motor_tilt_max : Maximum safe tilt
motor_tilt_forward : When tilt motor is set to this value it looks forward
motor_buffer_depth : Number of frames before an action in the real world will be seen
show_frame_rgb : Flag for showing RGB frame
show_frame_mask : Flag for showing masked frame
show_frame_track : Not used
print_coordinates : Flag for printing the XYZ coordinates of the balloon
print_rotation : Flag for printing the pan and tilt of the motors
print_info : Flag for printing various details
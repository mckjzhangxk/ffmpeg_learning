#查看支持 的输入设备,例如 avfoundation
ffmpeg -v quiet -devices

#查看设备索引值,-f 表示 文件格式
ffmpeg  -f avfoundation -list_devices true -i ""


# [AVFoundation indev @ 0x7fba2a411900] AVFoundation video devices:
# [AVFoundation indev @ 0x7fba2a411900] [0] FaceTime HD Camera
# [AVFoundation indev @ 0x7fba2a411900] [1] OBS Virtual Camera
# [AVFoundation indev @ 0x7fba2a411900] [2] Capture screen 0
# [AVFoundation indev @ 0x7fba2a411900] AVFoundation audio devices:
# [AVFoundation indev @ 0x7fba2a411900] [0] 外置麦克风
# [AVFoundation indev @ 0x7fba2a411900] [1] MacBook Pro麦克风
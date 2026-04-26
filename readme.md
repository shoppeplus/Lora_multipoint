tạo 4 folder để nạp code vào vi điều khiển thông qua extension flatform io của visualcode gồm có
-folder 1 là mạch master: gồm 1 esp32, 1 lora RA02. dùng để thu nhận tín hiệu từ slave thông qua lora. các chân cấu hình của mạch 
    #define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 2
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
-folder 2,3,4 là các mạch slave tương ứng với 3 mạch slave độc lập và có phần cứng giống nhau gồm có 1 stm32, 1 lora RA02, 1 mpu6050. mạch slave gồm các chân cấu hình như sau:
    #define LORA_SS    PA4
    #define LORA_RST   PB9
    #define LORA_DIO0  PB8
    #define LORA_SCK   PA5
    #define LORA_MISO  PA6
    #define LORA_MOSI  PA7
    // MPU6050
    #define MPU_ADDR        0x68
    #define PWR_MGMT_1      0x6B
    #define ACCEL_XOUT_H    0x3B
-hệ thống tôi muốn làm có chức năng thu thập dữ liệu chuyển động của cầu và xử lí dữ liệu đấy tại các slave sau đó truyền về master thông qua Lora.
hãy lên plan để và cho tôi idea để có thể giải quyết bài toán thực tế này 1 cách khả thi.tôi mong muốn có thể từ các số liệu của mpu6050 có thể tính toán được tần số dao động của cầu tại điểm đo.
output: mạch master có thể thu thập đủ thông tin truyền về từ 3 slave và không bị mất dữ liệu,lập cho tôi 1 cái plan implement để quan sát 

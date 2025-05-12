# Driver TCS34725
1. Đặng Minh Huynh - 22146317
2. Phạm Việt Hoàn - 22146307
3. Phạm Vũ Đức Hải - 22146301

## 1. Giới thiệu về TCS34725 và ứng dụng của nó

TCS34725 là một cảm biến màu (Color Sensor) do AMS sản xuất. Nó có thể đo cường độ ánh sáng đỏ (Red), xanh lá (Green), xanh dương (Blue) và tổng cường độ ánh sáng (Clear light). Cảm biến này tích hợp bộ lọc hồng ngoại (IR Filter) giúp tăng độ chính xác của việc đo màu dưới nhiều điều kiện ánh sáng khác nhau. Giao tiếp qua I2C giúp dễ dàng tích hợp với vi điều khiển như Arduino, STM32, Raspberry Pi,...

**Ứng dụng phổ biến:**
- Nhận dạng màu sắc trong các hệ thống phân loại sản phẩm.
- Robot tránh vạch hoặc nhận dạng vạch màu.
- Điều chỉnh ánh sáng thông minh dựa trên màu sắc môi trường.
- Cảm biến môi trường trong các thiết bị IoT.

---

## 2. Cách cài đặt TCS34725

### Yêu cầu phần cứng:
- Cảm biến TCS34725
- Raspberry Pi 5 Model B
- Dây nối

### Kết nối chân (sử dụng bộ I2C1)
| TCS34725 |   Pi 5  |
|----------|---------|
| VIN      | Pin 1   |
| GND      | Pin 6   |
| SDA      | Pin 3   |
| SCL      | Pin 5   |

### Cập nhật Device Tree cho file bcm2712-rpi-5-b.dtb
1. Dùng terminal và chuyển đến thư mục boot (hoặc boot/firmware tuỳ phiên bản): 
        cd /boot 
hoặc 
        cd /boot/firmware/
2. Dùng câu lệnh để đổi file dtb sang dts 
        sudo dtc -I dtb -O dts -o bcm2712-rpi-5-b.dts bcm2712-rpi-5-b.dtb
3. Truy cập vào file dts bằng cách nhập vào câu lệnh: "sudo nano dts bcm2712-rpi-5-b.dts"
4. Kiếm cụm từ (sử dụng Ctrl + W) aliases
5. Tìm đến dòng chứa cụm i2c1 (ví dụ: i2c1@74000), copy chúng và tìm kiếm
6. Ở dòng cuối cùng, thêm vào những dòng sau:

        tcs34725@29 {
                compatible = "taos,tcs34725";
                reg = <0x29>;
        };

7. Chuyển ngược lại sang file dts: "sudo dtc -I dts -O dtb -o bcm2712-rpi-5-b.dtb bcm2712-rpi-5-b.dts"
8. Lưu file lại và reboot (khởi động lại) bằng câu lệnh "sudo reboot"

### Cài đặt thư viện
1. Đảm bảo kết nối phần cứng TCS34725 với Raspberry Pi 5 
2. Tạo Makefile với nội dung như sau: 

        obj-m += tcs34725_driver.o
        KDIR = /lib/modules/$(shell uname -r)/build

        all:
	        make -C $(KDIR) M=$(shell pwd) modules
        clean: 
	        make -C $(KDIR) M=$(shell pwd) clean

3. Ngay trong thư mục chứa file driver và Makefile, mở terminal và dùng câu lệnh "make"
4. Tiếp theo cài đặt driver bằng: "sudo insmod tcs34725_driver.ko"
5. Khi không muốn sử dụng driver nữa, sử dụng câu lệnh "sudo rmmod tcs34725_driver" để gỡ, và câu lệnh "make clean" để xoá các file thành phẩm

### Tạo file code để tương tác với cảm biến sử dụng driver
Trong file code, lưu ý một số điểm sau đây:
- Thêm vào các dòng sau đây để khai báo địa chỉ device, các hàm ioctl và hàm cấu trúc (dùng để lưu trữ giá trị r g b sau khi normalized nếu sử dụng hàm) bên dưới khai báo thư viện: 
    #define DEVICE_PATH "/dev/tcs34725"

    #define TCS34725_IOCTL_MAGIC 't'
    #define TCS34725_IOCTL_READ_RED   _IOR(TCS34725_IOCTL_MAGIC, 2, int)
    #define TCS34725_IOCTL_READ_GREEN _IOR(TCS34725_IOCTL_MAGIC, 3, int)
    #define TCS34725_IOCTL_READ_BLUE  _IOR(TCS34725_IOCTL_MAGIC, 4, int)
    #define TCS34725_IOCTL_READ_RGB_NORMALIZED _IOR(TCS34725_IOCTL_MAGIC, 5, struct tcs34725_rgb)

    struct tcs34725_rgb {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };
- Thêm câu lệnh "int fd = open(DEVICE_PATH, RDONLY);" để mở driver đầu hàm main() và "close(fd)" để đóng driver ở cuối hàm main()
- Sử dụng câu lệnh "gcc tên_file_code -o tên_file_thành_phẩm" và dùng câu lệnh "sudo ./tên_file_thành_phẩm" để thực thi
---

## 3. Cách sử dụng các hàm IOCTL để tương tác trong hàm 
Cấu trúc hàm iotcl: "tcs34725_ioctl(struct file *file, unsigned int cmd, unsigned long arg)"
Trong đó: 
- struct file *file: tên của biến đã dùng để mở driver (như đã hướng dẫn ở trên là biến fd, có thể đổi tên biến bằng tên khác)
- unsigned int cmd: chọn hàm làm việc, bao gồm các hàm sau đây:
1. Hàm "TCS34725_IOCTL_READ_CLEAR": đọc giá trị thô của tổng cường độ ánh sáng
2. Hàm "TCS34725_IOCTL_READ_RED": đọc giá trị thô cường độ ánh sáng đỏ
3. Hàm "TCS34725_IOCTL_READ_GREEN": đọc giá trị thô cường độ ánh sáng xanh lá
4. Hàm "TCS34725_IOCTL_READ_BLUE": đọc giá trị thô cường độ ánh sáng xanh dương
5. Hàm "TCS34725_IOCTL_READ_RGB_NORMALIZED": đọc cả ba giá trị đỏ, xanh lá, xanh dương và chuẩn hoá nó về dạng tiêu chuẩn (0, 256)
- unsigned long arg: biến để lưu giá trị khi thực hiện các hàm, đối với hàm 1, 2, 3 và 4, biến đơn sẽ được trả về, còn đối với hàm 5, biến cấu trúc (bao gồm ba giá trị r, g, b) sẽ được trả về (có hướng dẫn cách khai báo hàm struct ở trên) 

## 4. Một số lỗi thường gặp khi chạy code
File code để tương tác với cảm biến khi chạy có thể xảy ra thông báo lỗi không tìm thấy device, điều này có thể do một số nguyên nhân sau: 
- Chỉnh sửa file dts chưa đúng: lưu ý viết đúng phần compatible (không khoảng trống) hoặc để ý các lỗi chính tả khi viết
- Module đã kết nối với Raspberry Pi 5, nhưng bị một driver khác chiếm dụng, muốn kiểm tra điều này thì sử dụng câu lệnh trên terminal: "i2cdetect -y 1". Nếu tại vị trí 0x29 xuất hiện chữ UU thì tức là module đã bị một driver khác sử dụng, khiến cho driver tcs34725_driver không được chạy hàm probe. Để sửa lỗi này, cần kiểm tra xem driver nào đã chiếm dụng, cho chúng vào blacklist sau đó reboot lại hệ thống

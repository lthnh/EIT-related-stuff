#include <CD74HC4067.h>
#include <TrueRMS.h>

#define LPERIOD 1000    // thời gian giữa các mẫu micro giây
#define ADC_INPUT A7    // xác định chân ADC đầu vào
#define RMS_WINDOW 100  // 40 mẫu, 2 khoảng thời gian ở 50HZ
#define NUM_OF_NODES 16 // Số điện cực đo

int loopNumber = 1;
unsigned long loopTimeTillNext = 0;
int adcVal;
int cnt = 0;
float voltageRange = 5.00; // Vp_p là 5V
int registerData;

Rms readRms;
CD74HC4067 MUX_1(22, 23, 24, 25); // khai báo chân s0 s1 s2 s3 trên mux
CD74HC4067 MUX_2(26, 27, 28, 29);
CD74HC4067 MUX_3(30, 31, 32, 33);
CD74HC4067 MUX_4(34, 35, 36, 37);

void setup() { Serial.begin(115200); }

void loop() {
  if (Serial.available() > 0) {
    char commandCharacter = Serial.read();
    switch (commandCharacter) {
    case 'S':
      delay(100);
      Cycle();
      break;
    }
  }
}

void Cycle() {
  readRms.begin(voltageRange, RMS_WINDOW, ADC_10BIT, BLR_ON, CNT_SCAN);
  readRms.start();
  loopTimeTillNext =
      micros() +
      LPERIOD; //  dặt khoảng thời gian cho vòng lặp tiếp theo, micro trả
               //  về micro giây kể từ arduino, bắt đầu chạy chương trình
  for (int i = 1; i <= loopNumber; i++) {
    for (int j = 0; j < NUM_OF_NODES; j++) {
      measurementsTake(MUX_1, MUX_2, MUX_3, MUX_4, j, NULL);
    }
  }
}

float *measurementsTake(CD74HC4067 src1, CD74HC4067 src2, CD74HC4067 volmtr1,
                        CD74HC4067 volmtr2, int node, float *measures) {
  // Đo dùng 2 node liền kề nha
  src1.channel(measurementsWrapAround(node));
  src2.channel(measurementsWrapAround(node + 1));

  const int measureCaps = node + 14; // Giới hạn số lần đo
  for (int t = node + 2; t <= measureCaps; t++) {
    volmtr1.channel(measurementsWrapAround(t));
    volmtr2.channel(measurementsWrapAround(t + 1));
    measurementsTakeAndSend();
  }

  if (measures)
    measures = NULL; // Tạm thời trả về giá trị NULL cho tiện, đợi mở rộng code
                     // sau này

  return measures; // Code không an toàn
}

int measurementsWrapAround(int node) { return node %= NUM_OF_NODES; }

void measurementsTakeAndSend() {
  for (int i = 0; i < 500; i++) {
    adcVal = analogRead(ADC_INPUT); // đọc giá trị adc và loại bỏ dc-offset
    readRms.update(adcVal);         // lấy lại 1 giá trị
    cnt++;
    if (cnt >= 500) // lặp lại đọc 1 mẫu
    {
      readRms.publish();
      Serial.print(readRms.rmsVal, 6);
      Serial.write(10); // Ghi '\n' để ngắt quãng các giá trị

      cnt = 0;
    }
  }
}
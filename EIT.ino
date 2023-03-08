#include <CD74HC4067.h>
#include <TrueRMS.h>

#define ADC_INPUT A7   // Xác định chân ADC đầu vào
#define RMS_WINDOW 100 // 40 mẫu, 2 khoảng thời gian ở 50HZ
#define TERMINATOR '\n' // Có thể cần nếu sau này đọc dữ liệu từ string
#define MUX_NUM_OF_OUTPUT_PINS 8 // Số chân của một con mux
#define MUX_ENABLE_PIN 2 // Chân EN của MUX nối với pin 2 của Arduino Due (Arm)
#define MUX_DELAY_PERIOD 200

typedef struct muxDual {
  CD74HC4067 *mux1; // Khi là nguồn dòng mux1 là source
  CD74HC4067 *mux2; // Khi là nguồn dòng mux2 là sink
} muxDual;

long nodeNumber = 0;
long loopNumber = 0;
int adcVal = 0;
int cnt = 0;
float voltageRange = 5.0; // Vp_p là 5V
int registerData = 0;

// 1 con mux chỉ có 8 chân
Rms readRms;
// Khai báo chân s0 s1 s2 s3 trên mux
CD74HC4067 muxSourcePositive1(22, 23, 24, 25);
CD74HC4067 muxSourceNegative1(26, 27, 28, 29);
CD74HC4067 muxVoltMeter11(30, 31, 32, 33);
CD74HC4067 muxVoltMeter12(34, 35, 36, 37);
CD74HC4067 muxSourcePositive2(38, 39, 40, 41);
CD74HC4067 muxSourceNegative2(42, 43, 44, 45);
CD74HC4067 muxVoltMeter21(46, 47, 48, 49);
CD74HC4067 muxVoltMeter22(50, 51, 52, 53);

muxDual muxSourceCurrent[32];
muxDual muxVoltMeter[32];

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(1000);

  while (!Serial || !Serial.available()) {
  }

  nodeNumber = Serial.parseInt(SKIP_ALL);
  serialFlushIncomingBuffer(); // Chuỗi gửi từ Arduino serial monitor có giá trị
                               // kết thúc mặc định là CR/LF
  // Code này đổi sang LF (\n)
  // Khi dùng method parseInt sẽ không đọc LF. Dẫn tới incoming buffer còn 1
  // byte làm code lỗi

  if (nodeNumber == 32) {
    for (int i = 0; i < 15; i++) {
      muxSourceCurrent[i].mux1 = &muxSourcePositive1;
      muxSourceCurrent[i].mux2 = &muxSourceNegative1;
      muxVoltMeter[i].mux1 = &muxVoltMeter11;
      muxVoltMeter[i].mux2 = &muxVoltMeter12;
    }
    // Cài nút 15 riêng vì ở đây cần hai con mux khác nhau
    muxSourceCurrent[15].mux1 = &muxSourcePositive1;
    muxSourceCurrent[15].mux2 = &muxSourceNegative2;
    muxVoltMeter[15].mux1 = &muxVoltMeter11;
    muxVoltMeter[15].mux2 = &muxVoltMeter22;
    for (int i = 16; i < 31; i++) {
      muxSourceCurrent[i].mux1 = &muxSourcePositive2;
      muxSourceCurrent[i].mux2 = &muxSourceNegative2;
      muxVoltMeter[i].mux1 = &muxVoltMeter21;
      muxVoltMeter[i].mux2 = &muxVoltMeter22;
    }
    // Cài nút 31 riêng vì ở đây cần hai con mux khác nhau
    muxSourceCurrent[15].mux1 = &muxSourcePositive1;
    muxSourceCurrent[31].mux1 = &muxSourcePositive2;
    muxSourceCurrent[31].mux2 = &muxSourceNegative1;
    muxVoltMeter[31].mux1 = &muxVoltMeter21;
    muxVoltMeter[31].mux2 = &muxVoltMeter12;
  }

  pinMode(MUX_ENABLE_PIN, OUTPUT);
}

void loop() {
  while ((Serial.available() > 0) && (loopNumber == 0)) {
    loopNumber = Serial.parseInt(SKIP_ALL);
  }

  if ((Serial.available() > 0) && (loopNumber > 0)) {
    char commandCharacter = Serial.read();
    switch (commandCharacter) {
    case 'S':
      cycle();
      loopNumber = 0;
      serialFlushIncomingBuffer();
      break;
    }
  }
}

void serialFlushIncomingBuffer(void) { Serial.readStringUntil(TERMINATOR); }

void cycle() {
  if (nodeNumber == 16) {
    digitalWrite(MUX_ENABLE_PIN, HIGH);
    delay(MUX_DELAY_PERIOD);
    readRms.begin(voltageRange, RMS_WINDOW, ADC_10BIT, BLR_ON, CNT_SCAN);
    readRms.start();
    for (int i = 0; i < loopNumber; i++) {
      for (int j = 0; j < nodeNumber; j++) {
        measurementProcessing(muxSourcePositive1, muxSourceNegative1,
                              muxVoltMeter11, muxVoltMeter12, j);
      }
    }
    delay(MUX_DELAY_PERIOD);
    digitalWrite(MUX_ENABLE_PIN, LOW);
  }

  if (nodeNumber == 32) {
    int loopCurrentNumber = 0;
    while (loopCurrentNumber < loopNumber) {
      for (int i = 0; i < nodeNumber; i++) {
        // Tạo nguồn dòng trong môi trường
        muxOpenChannel(muxSourceCurrent[i].mux1, measurementNodeWrapAround(i));
        muxOpenChannel(muxSourceCurrent[i].mux2,
                       measurementNodeWrapAround(i + 1));
        // Đo điện thế tại biên
        const int measureCaps = i + nodeNumber;
        for (int j = i; j < measureCaps; j++) {
          muxOpenChannel(muxVoltMeter[j].mux1, measurementNodeWrapAround(j));
          muxOpenChannel(muxVoltMeter[j].mux2,
                         measurementNodeWrapAround(j + 1));
          measurementTakeAndSend();
        }
      }
    }
  }
}

void muxOpenChannel(CD74HC4067 *mux, int channel) { mux->channel(channel); }

void measurementProcessing(CD74HC4067 src1, CD74HC4067 src2, CD74HC4067 volmtr1,
                           CD74HC4067 volmtr2, int node) {
  // Đo dùng 2 node liền kề nhau
  src1.channel(measurementNodeWrapAround(node));
  src2.channel(measurementNodeWrapAround(node + 1));

  const int measureCaps = node + 15; // Giới hạn số lần đo
  for (int t = node; t <= measureCaps; t++) {
    volmtr1.channel(measurementNodeWrapAround(t));
    volmtr2.channel(measurementNodeWrapAround(t + 1));
    measurementTakeAndSend();
  }
}

int measurementNodeWrapAround(int node) { return node %= nodeNumber; }

void measurementTakeAndSend() {
  for (int i = 0; i < 500; i++) {
    adcVal = analogRead(ADC_INPUT); // Đọc giá trị adc và loại bỏ dc-offset
    readRms.update(adcVal);         // Lấy lại 1 giá trị
    cnt++;
    if (cnt >= 500) // Lặp lại đọc 1 mẫu
    {
      readRms.publish();
      Serial.print(readRms.rmsVal, 6);
      Serial.write(10); // Ghi '\n' để ngắt quãng các giá trị

      cnt = 0;
    }
  }
}
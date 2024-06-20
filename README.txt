스마트홈 관리 시스템

김성빈(22100113)
최준혁(21900764)

프로젝트의 소스코드 리스트
READEME.txt
main.c
1) main 프로그램이 실행되는 파일
2) uart를 이용한 co2 ppm 측정 및 co2 level로 변환
3) 사운드 센서를 통해 소리 측정 및 소음 level로 변환

buttons.c
1) 각 버튼마다 인터럽트 발생, I2C LED 매트릭스에 출력
2) button1: 습도 출력
3) button2: 온도 출력
4) button3: co2 level에 따른 표정  출력
5) button4: 소음 level 출력

dht22_sensor.c
1) 온습도 센서 관련 코드: 온습도 데이터 가져옴

ht16k33_led.c
1) I2C LED 매트릭스 관련 코드

my_service.c
1) BLE 관련 코드

빌드하는 방법
1) VSCode extension - NRF Connect, West 설치
2) Project Root directory 열기
3) Build - prj.conf, nrf52840dk-nrf52840.overlay 선택

소스 작성자 정보 및 날짜
2024.06.20
김성빈(22100113)
최준혁(21900764)

다운로드받아 사용한 오픈소스 정보
https://github.com/UmileVX/IoT-Development-with-Nordic-Zephyr
https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/sensor/dht


Copyright (c) 2024 Kim Seongbin, Choi Junhyeok

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

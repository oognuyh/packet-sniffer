# Packet Sniffer
<img src="https://img.shields.io/badge/C-A8B9CC.svg?&style=for-the-badge&logo=C&logoColor=white"> <img src="https://img.shields.io/badge/Kali Linux-557C94.svg?&style=for-the-badge&logo=KaliLinux&logoColor=white"> <img src="https://img.shields.io/badge/Visual Studio Code-007ACC.svg?&style=for-the-badge&logo=VisualStudioCode&logoColor=white">

> TCP/IP 프로토콜 분석 프로그램 개발

PC 1대로 논리적 분리 환경을 구축하여 A, B를 운영하고 A에서 HTTP / DNS / ICMP를 요청하고 B에서 패킷을 캡쳐 후 로그로 저장 및 필터링 기능을 갖춘 프로그램을 구현

## Environment
* 구조도  
![structure][structure]
* 배치 다이어그램  
![batch-diagram][batch-diagram]

## Flow Chart
![flow-chart][flow-chart]

## Features
* HTTP, HTTPS, DNS, ICMP 패킷 캡쳐
* 패킷 캡쳐 중지
* 필터링
* 전체 패킷을 log.txt, 필터링된 패킷을 filter.txt로 출력

## How to use
* Installation
```
git clone https://github.com/oognuyh/packet-sniffer.git

cd packet-sniffer

gcc -o packet_sniffer packet_sniffer.c

./packet_sniffer
```
* Filtering  
    엔터키 입력 시 필터링 모드로 전환  
    필터링 모드에서 값을 주지 않을 시 종료
```
<http|https|dns|icmp|all> 
<http|https|dns|icmp|all> <ip>
<http|https|dns|icmp|all> <source ip|all> <dest ip|all>
```

## Screenshots
![main][main]

* Wireshark와 비교
![http][http]
![dns][dns]
![icmp][icmp]

## What i learned
* HTTP / DNS / ICMP 프로토콜에 대한 이해
* 각 프로토콜 별 패킷의 구조 및 이해
* Raw Socket에 대한 이해
* 3-Way Handshaking 동작 원리
* 네트워크 속 패킷의 송수신 과정에 대한 원리

[structure]: https://user-images.githubusercontent.com/48203569/124743933-31788f00-df59-11eb-87f9-b5561ecd7cba.png
[batch-diagram]: https://user-images.githubusercontent.com/48203569/124743937-32a9bc00-df59-11eb-8c5b-e4989b86e372.png
[flow-chart]: https://user-images.githubusercontent.com/48203569/124743938-32a9bc00-df59-11eb-91fe-8801a97f80b4.png
[main]: https://user-images.githubusercontent.com/48203569/124743941-33425280-df59-11eb-8094-4b9a0ae0523b.png
[http]: https://user-images.githubusercontent.com/48203569/124743943-33425280-df59-11eb-9fdd-73c1209bde8c.png
[dns]: https://user-images.githubusercontent.com/48203569/124743945-33dae900-df59-11eb-8da1-aacedb182ff0.png
[icmp]: https://user-images.githubusercontent.com/48203569/124743953-350c1600-df59-11eb-9154-0cfe62f0d558.png

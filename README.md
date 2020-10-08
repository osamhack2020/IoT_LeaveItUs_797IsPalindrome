# **맡기시오** (팀 *797은거꾸로해도797*)
![팀 로고](logo.png)

## 소개 및 설명 동영상
**맡기시오 (Leave It Us)**
는 코로나 시대에 하나의 휴대폰 보관함에 전 병력이 밀집하여 휴대폰을 불출/반납하며 생기는 접촉들을 최소화하기 위한 각 생활관별로 설치하는 휴대폰 보관함입니다. IoT와 Web기술을 접목하여 휴대폰에 부착된 NFC 인가 필증과 장착된 휴대폰들의 중량등을 인식한 결과를 처리하여 신뢰할 수 있는 반납 상태를 서버로 전송하기 때문에 당직계통이 일일히 인가된 유일한 휴대폰 제출 여부등을 확인하는데 발생하는 번거로움을 덜 수 있으며 웹페이지를 이용해 PC나 모바일 기기에서 원격으로 관리 가능합니다. 일일 병력 현황과 연동하여 유연한 반납 계획 수립 및 보관함 자체에 설치된 개폐 도어를 이용한 불출/반납 시간 관리같은 여러 부가기능 또한 지원합니다.

## 기능 설계
![아키텍처](https://github.com/osamhack2020/WEB_LeaveItUs_797IsPalindrome/raw/master/diagram/architecture.png)

## 컴퓨터 구성 / 필수 조건 안내
 -
 
## 기술 스택
### 백앤드
- Golang + Echo
- SQL + gorm
- JWT based Auth
- REST api
 
### 프론트앤드
- vuejs
- vuetify + vue-router(SPA)
- REST with axios 

### 하드웨어(IoT)
- Arduino mega + FreeRTOS
- LoRa
- modeling with TinkerCad
- NFC + FSR + Magnetic door sensor
- Auth with public key

## 설치 안내

## 프로젝트 사용법

## 팀 정보 (Team Information)
- 김정현 (kimdictor@gmail.com), Github Id: Dictor
- 김상윤 (ndkim11@naver.com), Github Id: ndkim11

## 저작권 및 사용권 정보
[AGPL](LICENSE)
```
Project "Leave It Us", Cellphone management system of consisted with web and iot.  
Copyright (C) 2020 JeongHyun Kim (kimdictor@gmail.com), Sangyoon Kim (ndkim11.naver.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
```
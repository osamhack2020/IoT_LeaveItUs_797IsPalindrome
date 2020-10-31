# 웹 백앤드 벤치마크 결과
동시 접속 15개, 접속 당 요청 10개로 진행했습니다. 

## 문 개폐 이벤트 목록 (GET /api/locker/door)
문 개폐 이벤트 전체 Read: 행 개수 2200여개, 페이로드 480KB
```
./cassowary run -u http://localhost:80/api/locker/door -c 15 -n 150

Starting Load Test with 150 requests using 15 concurrent users

 100% |████████████████████████████████████████| [4s:0s]            4.677901733s


 TCP Connect.....................: Avg/mean=0.33ms      Median=0.00ms   p(95)=1.00ms
 Server Processing...............: Avg/mean=444.13ms    Median=418.50ms p(95)=709.00ms
 Content Transfer................: Avg/mean=13.71ms     Median=0.00ms   p(95)=99.00ms

Summary: 
 Total Req.......................: 150
 Failed Req......................: 0
 DNS Lookup......................: 0.00ms
 Req/s...........................: 32.07
```

## 라커 목록 (GET /api/locker)
라커 목록 전체 Read: 행 개수 15개, 페이로드 14KB
```
./cassowary run -u http://localhost:80/api/locker -c 15 -n 150

Starting Load Test with 150 requests using 15 concurrent users

 100% |████████████████████████████████████████| [0s:0s]            442.296525ms


 TCP Connect.....................: Avg/mean=0.50ms      Median=0.00ms   p(95)=2.00ms
 Server Processing...............: Avg/mean=33.97ms     Median=24.00ms  p(95)=91.00ms
 Content Transfer................: Avg/mean=7.61ms      Median=0.00ms   p(95)=58.00ms

Summary: 
 Total Req.......................: 150
 Failed Req......................: 0
 DNS Lookup......................: 1.00ms
 Req/s...........................: 339.14
```

## 데이터 삽입 (POST /api/locker, tag, person)
```
========
315 requests are done
total elapsed time is 3.675432 second.
85.704208 req/s
========
```

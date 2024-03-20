# simplePoW

- [1. 통신 플로우 및 패킷 구조](#1-통신-플로우-및-패킷-구조)
  - [메인 서버와 워킹 서버 간의 통신 플로우](#11-메인-서버와-워킹-서버-간의-통신-플로우)
  - [패킷 페이로드 정의](#12-패킷-페이로드-정의)
  - [멀티 쓰레딩](#13-멀티-쓰레딩)
- [2. 단독 실행 vs. 분산 실행 결과](#2-단독-실행-vs-분산-실행-결과)
- [3. 네트워크 환경 설정](#3-네트워크-환경-설정)
- [+. Suggestions](#-suggestions)

## 1. 통신 플로우 및 패킷 구조

### 1.1 메인 서버와 워킹 서버 간의 통신 플로우

---

<img src="https://github.com/jxxxxharu/jxxxxharu/assets/80589294/bcef3b82-6173-4920-9fcd-278f2604c2a4" alt="Main server와 Working server간의 프로토콜 플로우" width=75% />

1. 메인 서버 시작: 메인 서버가 구동되며, 이후 워킹 서버들이 지정된 IP와 포트를 통해 접속한다.
2. TCP 연결: 워킹 서버들과 메인 서버 간 TCP 연결된다.
3. 챌린지 및 난이도 전송: 메인 서버는 챌린지와 난이도 설정 후, 이 정보를 워킹 서버들에게 전송한다.
4. PoW 작업 시작: 각 워킹 서버는 할당받은 시작 nonce 값으로부터 PoW 연산을 시작한다.
5. 결과 전송 및 중단 신호: 조건을 만족하는 nonce 값을 찾은 워킹 서버는 그 값을 메인 서버에 전송한다. 메인 서버는 첫 번째로 받은 결과를 채택하고, 나머지 워킹 서버에 중단 신호를 보낸다.

<!-- 먼저 Main Server 구동시키고 해당 IP와 Port를 통하여 두 Working Server가 접속하면 TCP 연결이 형성된다. Main Server에서 challenge와 난이도를 설정하고, 이 정보를 각 Working Server에 전송한다. 전송된 정보에는 각 워킹 서버가 시작할 탐색 시작 nonce 값(num)도 포함된다. 이후 각 Working Server는 주어진 num부터 시작해서 nonce값 탐색을 탐색하는 PoW(작업증명) 과정을 진행하게 된다. 이 과정에서 해시 연산을 통해 특정 조건을 만족하는 nonce 값을 찾게 되면, 그 값을 Main Server에게 전송한다. 이 때 Main Server는 두 Working Server 중 어느 하나로부터 값을 먼저 받게 되면, 그 값을 채택하고 이를 나머지 서버에게 알린다. (stop sign) -->

### 1.2 패킷 페이로드 정의

---

- Challenge:
  - 학번: 20192829||20192851||20192847
  - 이름: 박민수||윤성준||양조은
- 난이도: 7 or 8
- 탐색 시작 번호(num)
  - Working Server #1: 0 ~ 4,611,686,018,427,387,902
  - Working Server #2: 4,611,686,018,427,387,903 ~

<img src="https://github.com/jxxxxharu/jxxxxharu/assets/80589294/bc3bdb86-0f01-4383-b79e-2b5ebe94c18c" alt="image" style="width: 60%;" />

### 1.3 멀티 쓰레딩

---

워킹 서버는 멀티 쓰레딩을 통해 PoW 연산을 병렬로 처리한다. 구체적으로는 다음과 같은 쓰레드 구성을 사용한다. 쓰레드 개수는 본인 컴퓨터 환경에서 실험을 통해 가장 성능이 우수했던 5개로 세팅했다.

- <b>PoW 연산 쓰레드</b>: 총 5개의 쓰레드가 각자의 nonce 탐색 범위 내에서 동시에 작업 수행
- <b>스톱싸인 리스너 쓰레드</b>: 메인 서버로부터 "STOP" 신호를 수신 대기하며, 신호를 받으면 모든 PoW 연산 쓰레드를 중단시킴

Working Server는 멀티쓰레딩을 사용하여 PoW 연산을 병렬로 처리한다. 각 쓰레드에게 `start_num`부터 `LONG_MAX`까지의 구간을 쓰레드 수에 따라 균등하게 나누어 할당하고, 각 쓰레드가 고유한 nonce 범위 내에서 작업을 수행하도록 했다. 각 PoW 연산 쓰레드는 작업 완료 후 또는 "STOP" 신호 수신 시 종료된다.

별도의 쓰레드(스톱싸인 리스너 쓰레드)가 메인 서버로부터 "STOP" 신호를 기다린다.

## 2. 단독 실행 vs. 분산 실행 결과

아래의 표는 각 챌린지, 난이도에 따른 단독 실행과 분산 실행의 실행시간을 보여준다. "단독 실행"은 하나의 working server만 사용하고 "분산 실행"은 여러 working server를 사용하는 시나리오를 나타낸다.

- 난이도 7일 때의 실행시간 차이

  | 난이도 7 | 단독 실행(초) | 분산 실행(초) | 실행 시간 비율 (분산/단독) |
  | :------: | :-----------: | :-----------: | :------------------------: |
  | **학번** |    239.78     |     92.63     |            0.39            |
  | **이름** |     18.20     |     12.60     |            0.69            |

  학번 챌린지에서는 단독 실행 시 239.78초가 걸리는 반면, 분산 실행에서는 92.63초로 상당히 단축된 시간이 소요되었다. 실행 시간 비율은 0.39로, 분산 실행이 단독 실행에 비해 약 61% 단축하였다.
  반면에 이름 챌린지에서는 단독 실행이 18.20초, 분산 실행이 12.60초로 이 경우에서도 분산 실행이 더 효율적이고 단독 실행에 비해 약 31% 단축하였다.

- 난이도 8일 때의 실행시간 차이

  | 난이도 8 | 단독 실행(초) | 분산 실행(초) | 실행 시간 비율 (분산/단독) |
  | :------: | :-----------: | :-----------: | :------------------------: |
  | **학번** |    522.89     |    514.40     |            0.98            |
  | **이름** |    3242.58    |    1882.74    |            0.58            |

  학번 챌린지에서는 단독 실행 시 522.89초, 분산 실행 시 514.40초로 거의 비슷한 시간이 소
  요되었다. 실행 시간 비율은 0.98로 거의 동일하다.
  이름 챌린지에서는 단독 실행 시간이 3242.58초, 분산 실행 시간이 1882.74초로 분산 실행이 단독 실행에 비해 시간을 많이 줄였다. 실행 시간 비율은 0.58로, 분산 실행이 단독 실행에 비해 약 42%의 시간을 절약하였다.

결론적으로 비슷한 환경이라는 가정 하에서 분산 실행이 일반적으로 더 빠르다. 하지만 하드웨어 성능의 차이 등으로 무조건 분산 실행이 더 빠른 결과가 나오지는 않았다. 예를 들어 난이도 8 과 같은 경우 똑같은 nonce값을 같은 working server가 찾기 때문에 분산실행과 단독실행의 실행 시간 비율이 1에 가깝다.

## 3. 네트워크 환경 설정

<p>
외부 워킹 서버에서 특정 포트를 사용해 내부 네트워크에서 실행되는 가상 머신의 메인 서버와 통신할 수 있도록 하기 위해 다음 설정을 했다.

- 공유기 설정: 외부에서 내부 네트워크의 특정 IP와 포트로 접근할 수 있도록 포트 포워딩 설정
- 윈도우 방화벽: 특정 포트를 통한 인바운드 연결을 허용하도록 인바운드 규칙 생성
- 가상 머신 NAT 설정: 가상 머신의 네트워크 어댑터를 NAT 모드로 해 외부 연결 허용하고 포트 포워딩 설정

<img src="https://github.com/jxxxxharu/jxxxxharu/assets/80589294/b998522b-cb79-44c5-a4b3-958cb2bb14dc" alt="서버 간 통신 네트워크 구조" width="650px"><br>
ㄴ 서버 간 통신 네트워크 구조

</p>

<br><br>

<!-- <p align="center" style="color:gray">
    <figure style="text-align: center;">
        <img src="https://github.com/jxxxxharu/jxxxxharu/assets/80589294/8d37c7e5-a687-4632-a00d-135a1795d688" alt="공유기 포트포워드 설정" width="800px"><figcaption style="text-align: center;">공유기 포트포워드 설정</figcaption><br>
        <img src="https://github.com/jxxxxharu/jxxxxharu/assets/80589294/76a45c52-6d28-49b2-b76f-27830ae4a38f" alt="윈도우 내부 IP주소" width="800px"><figcaption style="text-align: center;">윈도우 내부 IP주소</figcaption><br>
        <img src="https://github.com/jxxxxharu/jxxxxharu/assets/80589294/3df514f2-ad8e-4dcf-bba4-a796f639e638" alt="방화벽 인바운드 포트 규칙" width="800px"><figcaption style="text-align: center;">방화벽 인바운드 포트 규칙</figcaption><br>
        <img src="https://github.com/jxxxxharu/jxxxxharu/assets/80589294/3ff00be7-0d12-47a2-9495-74d5387cad7e" alt="가상 OS - Network Adapter" width="800px"><figcaption style="text-align: center;">가상 OS - Network Adapter</figcaption><br>
        <img src="https://github.com/jxxxxharu/jxxxxharu/assets/80589294/1601578b-7db6-448c-b1c1-2b8184152ff5" alt="VMware Virtual Network Editor" width="500px"><figcaption style="text-align: center;">VMware Virtual Network Editor</figcaption><br>
    </figure>
</p> -->

## +. Suggestions

PoW 연산의 난이도와 목적에 따라 쓰레드 수를 동적으로 조정하는 전략도 고려할 수 있겠다.

# PB-Protocol_Storage
Primary-Backup (PB) replication protocol을 사용하는 key-value store 기반 replicated storage 구현


### 프로그램 요구사항
**A.** 실행시 인자를 입력해서 leader와 follower 역할을 구분할 수 있습니다. 1이면 leader, 0이면 follower

**B.** Leader는 get 명령어를 받으면 Key-value store를 key를 가지고 lookup한 후, 만약 key가 존재한다면 해당 key에 대한 value를 클라이언트에게 보냅니다. 만약 key가 존재하지 않는다면, “Key not found” 메시지를 클라이언트에게 보냅니다

**C.** Leader는 put 명령어를 받으면 Key-value store에 해당 key와 value를 추가/수정한 후 다른 follower들에게 이를 전파합니다.

**D.** Follower는 leader로부터 put 명령어를 받으면 값을 갱신 후 leader에게 ack을 회신합니다.

**E.** Leader는 모든 Follower들로부터 ack을 회신하였다면, client에게 reply를 보내 commit합니다.


### 프로그램 실행 결과
* Client
![client](https://github.com/s-jiun/PB-Protocol_Storage/assets/84860387/c7375893-6ecb-4cc5-8d8e-dcfe24d4b432)

* Leader
![client](https://github.com/s-jiun/PB-Protocol_Storage/assets/84860387/5a7406d0-7def-4475-ae08-0f7ceb08c9ff)

* Follower1
![follower1](https://github.com/s-jiun/PB-Protocol_Storage/assets/84860387/c5530617-ae0f-4fcc-afa1-b719fd93a9fe)

* Follower2
![follower2](https://github.com/s-jiun/PB-Protocol_Storage/assets/84860387/dda6bfd8-892c-4986-bf04-52614832f9d8)

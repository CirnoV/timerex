# TimerEX

TimerEx는 소스:엔진 커뮤니티 서버를 위한 [SourceMod](https://www.sourcemod.net/) 타이머 확장입니다. 정확한 타이밍과 일시정지 기능을 제공합니다.

## 정확한 타이밍

TimerEx는 자체 타이머 구현으로 기본 타이머보다 정확한 타이밍에 함수를 실행합니다.

### SourceMod 기본 타이머
[![SourceMod Timer](http://i3.ytimg.com/vi/-vztzMhe0ho/hqdefault.jpg)](https://www.youtube.com/watch?v=-vztzMhe0ho)

### TimerEx
[![TimerEx](http://i3.ytimg.com/vi/j8v3W-9X2I8/hqdefault.jpg)](https://www.youtube.com/watch?v=j8v3W-9X2I8)

## `Handle` 없음

TimerEx는 `Handle`을 생성하지 않으며 `KillTimer`, `TriggerTimer` 등의 함수 또한 지원하지 않습니다.

```sourcepawn
public void OnClientConnected(int client) {
  CreateTimerEx(1.0, Foo, GetClientSerial(client));
}

public Action Foo(int serial) {
  int client = GetClientFromSerial(serial);
  // ...
}
```

## `interval` 제한 없음

TimerEx는 기본 타이머와 달리 0.1초 미만의 시간을 지원합니다.
> ⚠️서버가 `tickrate` 기반으로 작동하기 때문에 실질적으로 `1 / tickrate`초 보다 작은 값으로 설정 할 수 없습니다.
> 따라서 반복 주기가 너무 짧은 타이머는 서버에 큰 부하를 줄 수 있으니 주의하시기 바랍니다.

```sourcepawn
// SourceMod 기본 타이머
CreateTimer(0.05, Foo);
// TimerEX
CreateTimerEx(0.05, Bar);

// interval 제한으로 0.1초 후에 발동
public Action Foo(Handle timer) { /* ... */ }

// 0.05초 후에 발동
public Action Bar() { /* ... */ }
```

## 타이머 채널 기반 일시정지

TimerEx는 타이머 생성 시에 채널을 지정할 수 있습니다. 각 채널은 독립적으로 작동하며 채널이 일시정지 되면 해당 채널의 모든 타이머가 정지됩니다.

```sourcepawn
int serial = GetClientSerial(client);
CreateTimerEx(1.0, Foo, serial, .channel = serial);
PauseTimerChannel(serial);
CreateTimerEx(3.0, Resume, serial);

public Action Foo(int serial) { /* ... */ }

public Action Resume(int serial) {
  ResumeTimerChannel(serial);
}
```

### 시간 정지 구현 예시
![hp3](https://user-images.githubusercontent.com/17797795/123240035-ea32dd00-d51a-11eb-9a60-e8fc0f6c472a.gif)

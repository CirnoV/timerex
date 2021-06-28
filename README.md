# TimerEX

[![CI](https://github.com/CirnoV/timerex/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/CirnoV/timerex/actions/workflows/ci.yml)

TimerEx는 소스:엔진 커뮤니티 서버를 위한 [SourceMod](https://www.sourcemod.net/)용 타이머 확장입니다. 정확한 타이밍과 일시정지 기능을 제공합니다.

## 정확한 타이밍

TimerEx는 SourceMod 기본 타이머보다 빠르고 정확한 타이머 구현을 목표로 제작되었으며, 엔진 시간에 따른 밀림 현상이 없습니다.

### SourceMod 기본 타이머 (YouTube)
[![SourceMod Timer](http://i3.ytimg.com/vi/-vztzMhe0ho/hqdefault.jpg)](https://www.youtube.com/watch?v=-vztzMhe0ho)

### TimerEx (YouTube)
[![TimerEx](http://i3.ytimg.com/vi/j8v3W-9X2I8/hqdefault.jpg)](https://www.youtube.com/watch?v=j8v3W-9X2I8)

## `Handle` 없음

TimerEx는 `Handle`을 생성하지 않으며 `KillTimer`, `TriggerTimer` 등의 함수 또한 지원하지 않습니다.

```sourcepawn
public void OnClientConnected(int client) {
  // Warning: tag mismatch
  Handle timer = CreateTimerEx(1.0, Foo, GetClientSerial(client));
}

public Action Foo(int serial) { /* ... */ }
```

## `interval` 제한 없음

최소 간격이 0.1초인 기본 타이머와 달리 TimerEx에는 이러한 제한이 없습니다. 그러나 서버가 `tickrate` 기반으로 작동하기 때문에 실질적으로 `1 / tickrate`초 보다 작은 값으로 설정 할 수 없으며, 반복 주기가 너무 짧은 타이머는 서버에 큰 부하를 줄 수 있으니 주의하시기 바랍니다.

```sourcepawn
// SourceMod 기본 타이머
CreateTimer(0.05, Foo);
// TimerEX
CreateTimerEx(0.05, Bar);

// interval 제한으로 0.1초 뒤에 호출
public Action Foo(Handle timer) { /* ... */ }

// 0.05초 뒤에 호출
public Action Bar() { /* ... */ }
```

## 타이머 채널 기반 일시정지

TimerEx는 타이머 생성 시에 채널을 지정할 수 있습니다. 각 채널은 독립적으로 작동하며 채널이 일시정지 되면 해당 채널의 모든 타이머가 정지됩니다.

```sourcepawn
int serial = GetClientSerial(client);
CreateTimerEx(1.0, Foo, serial, .channel = serial);
PauseTimerChannel(serial);
CreateTimerEx(3.0, Resume, serial);

// 4초 뒤에 호출
public Action Foo(int serial) { /* ... */ }

public Action Resume(int serial) {
  ResumeTimerChannel(serial);
}
```

### 시간 정지 구현 예시
![hp3](https://user-images.githubusercontent.com/17797795/123240035-ea32dd00-d51a-11eb-9a60-e8fc0f6c472a.gif)

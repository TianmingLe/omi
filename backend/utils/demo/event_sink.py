import asyncio
import json
import time
from collections import deque
from typing import Any, Protocol


def _ts() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def _log(level: str, event: str, **fields: Any) -> None:
    payload: dict[str, Any] = {"ts": _ts(), "level": level, "service": "event-sink", "event": event}
    payload.update(fields)
    print(json.dumps(payload, ensure_ascii=False), flush=True)


class EventSink(Protocol):
    async def emit(self, event: dict[str, Any]) -> None: ...


class MemorySink:
    def __init__(self, maxlen: int = 1000):
        self._buf: deque[dict[str, Any]] = deque(maxlen=maxlen)
        self._lock = asyncio.Lock()

    async def emit(self, event: dict[str, Any]) -> None:
        async with self._lock:
            self._buf.append(dict(event))

    def snapshot(self) -> list[dict[str, Any]]:
        return list(self._buf)


class RedisSink:
    def __init__(self, redis: Any, stream: str, maxlen: int = 1000):
        self._redis = redis
        self._stream = stream
        self._maxlen = maxlen

    async def emit(self, event: dict[str, Any]) -> None:
        payload = json.dumps(event, ensure_ascii=False)
        await self._redis.xadd(self._stream, {"payload": payload}, maxlen=self._maxlen, approximate=True)


class CompositeSink:
    def __init__(self, sinks: list[EventSink]):
        self._sinks = list(sinks)

    async def emit(self, event: dict[str, Any]) -> None:
        alive: list[EventSink] = []
        for s in self._sinks:
            try:
                await s.emit(event)
                alive.append(s)
            except Exception as e:
                _log("warn", "sink_emit_failed", error=str(e))
        self._sinks = alive


_REDIS = None


def _get_redis_client():
    global _REDIS
    if _REDIS is not None:
        return _REDIS
    try:
        import os

        redis_url = os.environ.get("REDIS_URL")
        if not redis_url:
            return None
        import redis.asyncio as redis

        _REDIS = redis.from_url(redis_url, decode_responses=True)
        return _REDIS
    except Exception as e:
        _log("warn", "redis_sink_unavailable", error=str(e))
        return None


def build_event_sink(trace_id: str, memory_maxlen: int = 1000) -> tuple[EventSink, MemorySink]:
    memory = MemorySink(maxlen=memory_maxlen)
    r = _get_redis_client()
    if r is None:
        return memory, memory
    return CompositeSink([RedisSink(redis=r, stream=f"demo:events:{trace_id}", maxlen=1000), memory]), memory


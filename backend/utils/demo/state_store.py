import asyncio
import json
import os
import time
from typing import Any


def _ts() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def _log(level: str, event: str, **fields: Any) -> None:
    payload: dict[str, Any] = {"ts": _ts(), "level": level, "service": "demo-state", "event": event}
    payload.update(fields)
    print(json.dumps(payload, ensure_ascii=False), flush=True)


class DemoState:
    def __init__(self):
        self._lock = asyncio.Lock()
        self._sessions: dict[str, dict[str, Any]] = {}
        self._results: dict[str, dict[str, Any]] = {}
        self._session_trace: dict[str, str] = {}

        self._redis = None
        redis_url = os.environ.get("REDIS_URL")
        if redis_url:
            try:
                import redis.asyncio as redis

                self._redis = redis.from_url(redis_url, decode_responses=True)
            except Exception as e:
                _log("warn", "redis_unavailable", error=str(e))
                self._redis = None

    async def close(self) -> None:
        if self._redis is None:
            return
        try:
            await self._redis.close()
            await self._redis.connection_pool.disconnect()
        except Exception:
            return

    async def _redis_set_json(self, key: str, value: dict[str, Any]) -> None:
        if self._redis is None:
            return
        try:
            await self._redis.set(key, json.dumps(value, ensure_ascii=False))
        except Exception as e:
            _log("warn", "redis_set_failed", key=key, error=str(e))

    async def _redis_get_json(self, key: str) -> dict[str, Any] | None:
        if self._redis is None:
            return None
        try:
            raw = await self._redis.get(key)
            if not raw:
                return None
            return json.loads(raw)
        except Exception as e:
            _log("warn", "redis_get_failed", key=key, error=str(e))
            return None

    async def bind_trace(self, session_id: str, trace_id: str) -> None:
        async with self._lock:
            self._session_trace[session_id] = trace_id

    async def get_trace(self, session_id: str) -> str | None:
        async with self._lock:
            return self._session_trace.get(session_id)

    async def record_feature_chunk(self, session_id: str, total_bytes: int, offset: int, size: int) -> None:
        async with self._lock:
            sess = self._sessions.setdefault(session_id, {"total": total_bytes, "ranges": []})
            sess["total"] = int(total_bytes)
            sess["ranges"].append([int(offset), int(offset) + int(size)])

    async def missing_ranges(self, session_id: str) -> list[list[int]]:
        async with self._lock:
            sess = self._sessions.get(session_id)
            if not sess:
                return [[0, 0]]
            total = int(sess.get("total") or 0)
            ranges = sorted(sess.get("ranges") or [])
            merged: list[list[int]] = []
            for s, e in ranges:
                if not merged:
                    merged.append([s, e])
                    continue
                ps, pe = merged[-1]
                if s <= pe:
                    merged[-1][1] = max(pe, e)
                else:
                    merged.append([s, e])
            missing: list[list[int]] = []
            cur = 0
            for s, e in merged:
                if s > cur:
                    missing.append([cur, s])
                cur = max(cur, e)
            if cur < total:
                missing.append([cur, total])
            return missing

    async def set_resume_cut(self, session_id: str, last_ack_offset: int) -> None:
        async with self._lock:
            sess = self._sessions.setdefault(session_id, {"total": 0, "ranges": []})
            sess["resume_cut"] = int(last_ack_offset)
        await self._redis_set_json(f"demo:resume_cut:{session_id}", {"last_ack_offset": int(last_ack_offset)})

    async def get_resume_cut(self, session_id: str) -> int:
        async with self._lock:
            sess = self._sessions.get(session_id) or {}
            if "resume_cut" in sess:
                return int(sess["resume_cut"])
        raw = await self._redis_get_json(f"demo:resume_cut:{session_id}")
        if raw and "last_ack_offset" in raw:
            return int(raw["last_ack_offset"])
        return 0

    async def mark_relay_inited(self, session_id: str) -> None:
        async with self._lock:
            sess = self._sessions.setdefault(session_id, {"total": 0, "ranges": []})
            sess["relay_inited"] = True
        if self._redis is None:
            return
        try:
            await self._redis.sadd("demo:relay_inited", session_id)
        except Exception as e:
            _log("warn", "redis_sadd_failed", error=str(e))

    async def is_relay_inited(self, session_id: str) -> bool:
        async with self._lock:
            sess = self._sessions.get(session_id) or {}
            if sess.get("relay_inited") is True:
                return True
        if self._redis is None:
            return False
        try:
            return bool(await self._redis.sismember("demo:relay_inited", session_id))
        except Exception as e:
            _log("warn", "redis_sismember_failed", error=str(e))
            return False

    async def set_result(self, trace_id: str, session_id: str, payload: dict[str, Any], events: list[dict[str, Any]]) -> None:
        result = {"status": "done", "trace_id": trace_id, "session_id": session_id, "payload": payload, "events": events}
        async with self._lock:
            self._results[trace_id] = result
        await self._redis_set_json(f"demo:result:{trace_id}", result)

    async def get_result(self, trace_id: str) -> dict[str, Any] | None:
        async with self._lock:
            if trace_id in self._results:
                return self._results[trace_id]
        return await self._redis_get_json(f"demo:result:{trace_id}")


_STATE: DemoState | None = None


def get_demo_state() -> DemoState:
    global _STATE
    if _STATE is None:
        _STATE = DemoState()
    return _STATE


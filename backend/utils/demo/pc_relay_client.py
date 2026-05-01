import asyncio
import base64
import json
import time
import zlib
from typing import Any

import websockets

from utils.demo.event_sink import EventSink, MemorySink, _ts


def _crc32_u32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


class PcRelayClient:
    def __init__(self, relay_url: str, sink: EventSink | None = None, max_retries: int = 5):
        self._relay_url = relay_url
        self._sink = sink or MemorySink(maxlen=1000)
        self._max_retries = max_retries

    async def _emit_event(self, op: str, **fields: Any) -> None:
        event: dict[str, Any] = {"ts": _ts(), "op": op}
        event.update(fields)
        try:
            await self._sink.emit(event)
        except Exception:
            return

    async def session_init(self, session_id: str, total_bytes: int, frame_type: int = 2) -> None:
        t0 = time.time()
        retry_count = 0
        while True:
            try:
                async with websockets.connect(self._relay_url) as ws:
                    await ws.send(
                        json.dumps(
                            {
                                "op": "session_init",
                                "session_id": session_id,
                                "frame_type": int(frame_type),
                                "total_bytes": int(total_bytes),
                            }
                        )
                    )
                    ack = json.loads(await ws.recv())
                    if not ack.get("accepted"):
                        raise RuntimeError("session_init_rejected")
                await self._emit_event(
                    "session_init",
                    session_id=session_id,
                    seq=0,
                    retry_count=retry_count,
                    elapsed_ms=int((time.time() - t0) * 1000),
                )
                return
            except Exception:
                retry_count += 1
                if retry_count > self._max_retries:
                    raise
                await asyncio.sleep(min(0.1 * (2**retry_count), 1.0))

    async def chunk(self, session_id: str, offset: int, data: bytes) -> int:
        t0 = time.time()
        retry_count = 0
        while True:
            try:
                async with websockets.connect(self._relay_url) as ws:
                    await ws.send(
                        json.dumps(
                            {
                                "op": "chunk",
                                "session_id": session_id,
                                "offset": int(offset),
                                "data": base64.b64encode(data).decode("ascii"),
                                "crc32": _crc32_u32(data),
                            }
                        )
                    )
                    ack = json.loads(await ws.recv())
                    if ack.get("op") != "chunk_ack":
                        raise RuntimeError("chunk_not_acked")
                    next_offset = int(ack.get("next_offset", offset + len(data)))
                await self._emit_event(
                    "chunk",
                    session_id=session_id,
                    seq=0,
                    retry_count=retry_count,
                    offset=int(offset),
                    bytes=len(data),
                    elapsed_ms=int((time.time() - t0) * 1000),
                )
                return next_offset
            except Exception:
                retry_count += 1
                if retry_count > self._max_retries:
                    raise
                await asyncio.sleep(min(0.1 * (2**retry_count), 1.0))

    async def session_resume(self, session_id: str, last_ack_offset: int) -> list[list[int]]:
        t0 = time.time()
        retry_count = 0
        while True:
            try:
                async with websockets.connect(self._relay_url) as ws:
                    await ws.send(
                        json.dumps(
                            {
                                "op": "session_resume",
                                "session_id": session_id,
                                "last_ack_offset": int(last_ack_offset),
                            }
                        )
                    )
                    state = json.loads(await ws.recv())
                    missing = state.get("missing_ranges") or []
                await self._emit_event(
                    "session_resume",
                    session_id=session_id,
                    seq=0,
                    retry_count=retry_count,
                    elapsed_ms=int((time.time() - t0) * 1000),
                )
                return missing
            except Exception:
                retry_count += 1
                if retry_count > self._max_retries:
                    raise
                await asyncio.sleep(min(0.1 * (2**retry_count), 1.0))

    async def session_complete_and_wait_result(self, session_id: str) -> dict[str, Any]:
        t0 = time.time()
        retry_count = 0
        while True:
            try:
                async with websockets.connect(self._relay_url) as ws:
                    await ws.send(json.dumps({"op": "session_complete", "session_id": session_id, "sha256": ""}))
                    msg = json.loads(await ws.recv())
                    if msg.get("op") != "result":
                        raise RuntimeError("result_missing")
                    payload = msg.get("payload") or {}
                elapsed_ms = int((time.time() - t0) * 1000)
                await self._emit_event(
                    "session_complete",
                    session_id=session_id,
                    seq=0,
                    retry_count=retry_count,
                    elapsed_ms=elapsed_ms,
                )
                await self._emit_event(
                    "result",
                    session_id=session_id,
                    seq=0,
                    retry_count=retry_count,
                    elapsed_ms=elapsed_ms,
                )
                return payload
            except Exception:
                retry_count += 1
                if retry_count > self._max_retries:
                    raise
                await asyncio.sleep(min(0.1 * (2**retry_count), 1.0))


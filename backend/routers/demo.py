import base64
import os
import time

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel

from utils.demo.event_sink import build_event_sink
from utils.demo.pc_relay_client import PcRelayClient
from utils.demo.state_store import get_demo_state

router = APIRouter()


class IngestFeature(BaseModel):
    trace_id: str
    scenario: str
    session_id: str
    payload_b64: str
    chunk_offset: int = 0
    chunk_total: int = 128


class IngestAudio(BaseModel):
    trace_id: str
    session_id: str | None = None
    payload_b64: str


@router.get("/healthz")
def healthz():
    return {"ok": True}


@router.get("/demo/healthz")
def demo_healthz():
    return {"ok": True}


@router.post("/demo/ingest/audio")
async def ingest_audio(body: IngestAudio):
    raw = base64.b64decode(body.payload_b64)
    if not raw:
        raise HTTPException(status_code=400, detail="empty_audio")
    return {"ok": True, "bytes": len(raw)}


async def _emit_warning_event(trace_id: str, message: str) -> None:
    sink, _ = build_event_sink(trace_id)
    await sink.emit({"ts": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), "op": "warn", "message": message})


@router.post("/demo/ingest/feature")
async def ingest_feature(body: IngestFeature):
    t0 = time.time()
    relay_url = os.environ.get("PC_RELAY_URL")
    if not relay_url:
        raise HTTPException(status_code=500, detail="missing_pc_relay_url")

    raw = base64.b64decode(body.payload_b64)
    if not raw:
        raise HTTPException(status_code=400, detail="empty_feature")

    total = int(body.chunk_total)
    if total <= 0:
        raise HTTPException(status_code=400, detail="invalid_total")

    state = get_demo_state()
    await state.bind_trace(body.session_id, body.trace_id)
    await state.record_feature_chunk(body.session_id, total_bytes=total, offset=int(body.chunk_offset), size=len(raw))

    sink, memory_sink = build_event_sink(body.trace_id)
    client = PcRelayClient(relay_url=relay_url, sink=sink)

    if not await state.is_relay_inited(body.session_id):
        await client.session_init(session_id=body.session_id, total_bytes=total, frame_type=2)
        await state.mark_relay_inited(body.session_id)

    await client.chunk(session_id=body.session_id, offset=int(body.chunk_offset), data=raw)

    if body.scenario == "resume" and int(body.chunk_offset) == 0:
        await state.set_resume_cut(body.session_id, int(body.chunk_offset) + len(raw))

    missing = await state.missing_ranges(body.session_id)
    if not missing:
        payload = await client.session_complete_and_wait_result(session_id=body.session_id)
        await state.set_result(body.trace_id, body.session_id, payload, memory_sink.snapshot())

    elapsed_ms = int((time.time() - t0) * 1000)
    try:
        await sink.emit({"ts": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), "op": "ingest_feature", "session_id": body.session_id, "seq": 0, "retry_count": 0, "elapsed_ms": elapsed_ms})
    except Exception:
        await _emit_warning_event(body.trace_id, "sink_emit_failed")

    return {"ok": True}


@router.get("/demo/relay/state/{session_id}")
async def relay_state(session_id: str):
    relay_url = os.environ.get("PC_RELAY_URL")
    if not relay_url:
        raise HTTPException(status_code=500, detail="missing_pc_relay_url")

    state = get_demo_state()
    last_ack = await state.get_resume_cut(session_id)

    trace_id = await state.get_trace(session_id) or ("trace-" + session_id)
    sink, _ = build_event_sink(trace_id)
    client = PcRelayClient(relay_url=relay_url, sink=sink)
    missing = await client.session_resume(session_id=session_id, last_ack_offset=int(last_ack))
    return {"missing_ranges": missing}


@router.get("/demo/result/{trace_id}")
async def get_result(trace_id: str):
    state = get_demo_state()
    res = await state.get_result(trace_id)
    if not res:
        return {"status": "pending"}
    return res


import hashlib
import json
import os
import uuid

from google.cloud import firestore
from google.auth.credentials import AnonymousCredentials

class _MockCollection:
    def stream(self):
        return []


class _MockFirestore:
    def collection(self, *_args, **_kwargs):
        return _MockCollection()


def _init_firestore_client():
    emulator = os.environ.get("FIRESTORE_EMULATOR_HOST")
    if emulator:
        project = os.environ.get("FIRESTORE_PROJECT_ID", "demo-project")
        return firestore.Client(project=project, credentials=AnonymousCredentials())

    if os.environ.get("OMI_DEMO") == "1":
        return _MockFirestore()

    if os.environ.get("SERVICE_ACCOUNT_JSON"):
        service_account_info = json.loads(os.environ["SERVICE_ACCOUNT_JSON"])
        with open("google-credentials.json", "w") as f:
            json.dump(service_account_info, f)

    return firestore.Client()


db = _init_firestore_client()


def get_users_uid():
    users_ref = db.collection('users')
    return [str(doc.id) for doc in users_ref.stream()]


def document_id_from_seed(seed: str) -> uuid.UUID:
    """Avoid repeating the same data"""
    seed_hash = hashlib.sha256(seed.encode('utf-8')).digest()
    generated_uuid = uuid.UUID(bytes=seed_hash[:16], version=4)
    return str(generated_uuid)

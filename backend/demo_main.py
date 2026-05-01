import logging

from fastapi import FastAPI

from routers import demo

logging.basicConfig(level=logging.INFO)


def create_app() -> FastAPI:
    app = FastAPI()
    app.include_router(demo.router)
    return app


app = create_app()

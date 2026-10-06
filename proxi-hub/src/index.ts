import { Hono } from "hono";

import { startBroker } from "./broker";
import ProxiRouter from "$/routers";
import DummyRouter from "$/routers/dummy";

const app = new Hono();

app.route("/bar", ProxiRouter);
app.route("/led", DummyRouter);

app.get("/", (c) => {
    return c.text("Hello Hono!");
});

await startBroker();
export default app;

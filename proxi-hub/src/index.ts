import { Hono } from "hono";

import { MQTTBroker } from "./broker";
import ProxiRouter from "$/routers";

const app = new Hono();
const broker = new MQTTBroker();

app.route("/bar", ProxiRouter);

app.get("/", (c) => {
    return c.text("Hello Hono!");
});

export default app;

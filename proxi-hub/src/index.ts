import { Hono } from "hono";

import { startBroker } from "./broker";
import FriendsRouter from "$/routers/friends";
import ProxiRouter from "$/routers/proxi";

const app = new Hono();
app.route("/friends", FriendsRouter);
app.route("/proxi", ProxiRouter);

app.get("/", (c) => {
    return c.text("Hello Hono!");
});


await startBroker();
export default app;

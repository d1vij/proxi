import * as v from "valibot";
import { Hono } from "hono";
import { vValidator } from "@hono/valibot-validator";
import { ProxiID } from "$/commonSchemas";
import db from "$/db";
import { ProxiInfoInsertSchema, proxiInfoTable } from "$/db/schema"

const router = new Hono();

router.post("/register", vValidator("json", ProxiInfoInsertSchema), async (c) => {
    const data = c.req.valid("json");
    await db.insert(proxiInfoTable).values(data);

    return c.json({
        success: true
    })
})

router.get("/all", async (c) => {
    const allProxies = db.select().from(proxiInfoTable).all();
    return c.json(allProxies);
})


export default router;

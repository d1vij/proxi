import * as v from "valibot";
import { vValidator } from "@hono/valibot-validator";
import { Hono } from "hono"

const ToggleBodySchema = v.object({
    current: v.boolean()
})

export default new Hono()
    .get("/on", c => c.json({ led: true }))
    .get("/off", c => c.json({ led: false }))
    .post("/toggle", vValidator("json", ToggleBodySchema), (c) => {
        const { current } = c.req.valid("json");
        return c.json(
            { led: !current }
        );
    })

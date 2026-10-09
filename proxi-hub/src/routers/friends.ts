import { objectify } from "radashi";
import * as v from "valibot";
import { Hono } from "hono";
import { vValidator } from "@hono/valibot-validator";

import db from "$/db";
import { proxiInfoTable, friendsTable, ProxiFriendInsertSchema } from "$/db/schema";
import { ProxiID } from "$/commonSchemas";
import { eq, inArray, or } from "drizzle-orm";

const router = new Hono();

export const GetSelfInfoSchema = v.object({
    proxi_id: ProxiID,
});

router.get("/:proxi_id", vValidator("param", GetSelfInfoSchema), async (c) => {
    console.log('got');
    const { proxi_id } = c.req.valid("param");
    const friendIdResult = db
        .select()
        .from(friendsTable)
        .where(
            or(
                eq(friendsTable.proxiA, proxi_id),
                eq(friendsTable.proxiB, proxi_id),
            ),
        )
        .all();

    const friendsId: string[] = friendIdResult.map((r) =>
        proxi_id === r.proxiA ? r.proxiB : r.proxiA,
    );
    const friendNameResult = db.select().from(proxiInfoTable).where(inArray(proxiInfoTable.proxiId, friendsId)).all();

    return c.json(objectify(friendNameResult, o => o.proxiId, o => o.proxiName));
});


router.post("/", vValidator("json", ProxiFriendInsertSchema), async (c) => {
    const data = c.req.valid("json");
    await db.insert(friendsTable).values(data);

    return c.json({ success: true });
})

export default router;

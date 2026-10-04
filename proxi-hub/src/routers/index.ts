import * as v from "valibot";
import { Hono } from "hono";
import { vValidator } from "@hono/valibot-validator";

import db from "$/db";
import { proxiInfoTable, friendsTable } from "$/db/schema";
import { ProxiID } from "$/commonSchemas";
import { eq, or } from "drizzle-orm";

const router = new Hono();

export const GetSelfInfoSchema = v.object({
    proxi_id: ProxiID,
});

router.get("/:proxi_id", vValidator("query", GetSelfInfoSchema), async (c) => {
    const { proxi_id } = c.req.valid("query");
    const result = db
        .select()
        .from(friendsTable)
        .where(
            or(
                eq(friendsTable.proxiA, proxi_id),
                eq(friendsTable.proxiB, proxi_id),
            ),
        )
        .all();

    const friendsId: string[] = result.map((r) =>
        proxi_id === r.proxiA ? r.proxiB : r.proxiA,
    );

    return c.json(friendsId);
});

export default router;

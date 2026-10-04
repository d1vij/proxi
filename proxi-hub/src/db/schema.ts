import { createInsertSchema } from "drizzle-orm/valibot";
import {
    sqliteTable,
    integer,
    text,
    primaryKey,
    check,
    index,
} from "drizzle-orm/sqlite-core";
import { sql } from "drizzle-orm";

export const proxiInfoTable = sqliteTable("proxi_info", {
    proxiId: text().primaryKey(),
});

export const ProxiInfoInsertSchema = createInsertSchema(proxiInfoTable);

export const friendsTable = sqliteTable(
    "proxi_friend",
    {
        proxiA: text("proxi_a")
            .notNull()
            .references(() => proxiInfoTable.proxiId),
        proxiB: text("proxi_b")
            .notNull()
            .references(() => proxiInfoTable.proxiId),
    },
    (t) => [
        primaryKey({ columns: [t.proxiA, t.proxiB] }),
        check("proxi_order", sql`${t.proxiA} < ${t.proxiB}`),
        index("proxi_friend_b_index").on(t.proxiB),
    ],
);

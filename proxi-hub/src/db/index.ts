import { Database } from "bun:sqlite";
import { drizzle } from "drizzle-orm/bun-sqlite";

export const db = drizzle({
    client: new Database(process.env.DB!),
});

db.run("PRAGMA foreign_keys = ON");

export default db;

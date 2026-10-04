import * as v from "valibot";

export const ProxiID = v.pipe(v.string(), v.uuid());

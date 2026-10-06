import { Aedes, AuthenticateHandler } from "aedes";

import { createServer, Server } from "node:net";

const authenticateHandler: AuthenticateHandler = () => { }

const broker = await Aedes.createBroker();
const server = createServer(broker.handle);

export async function startBroker(port = 1883) {
    return server.listen(port)
}
export function getBroker() {
    return broker;
}

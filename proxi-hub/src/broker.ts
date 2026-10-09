import { Aedes } from "aedes";
import { createServer, Server } from "node:net";

const broker: Aedes = await Aedes.createBroker(); // Note lowercase factory call

const server: Server = createServer(broker.handle);

// Bind to "0.0.0.0" so external Wi-Fi clients (Arduino) can connect
export function startBroker(port = 4000, host = "0.0.0.0"): Promise<void> {
    return new Promise((resolve, reject) => {
        server.listen(port, host, () => {
            console.log(`MQTT Broker listening on ${host}:${port}`);
            resolve();
        });

        server.on("error", (err) => {
            reject(err);
        });
    });
}

export function getBroker(): Aedes {
    return broker;
}

export function stopBroker(): Promise<void> {
    return new Promise((resolve) => {
        broker.close(() => {
            server.close(() => {
                console.log("MQTT Broker stopped");
                resolve();
            });
        });
    });
}

broker.on("client", c => console.log("connect", c.id));
broker.on("clientDisconnect", c => console.log("disconnect", c.id));
broker.on("clientError", (c, e) => console.log("clientError", c.id, e.message));
broker.on("connectionError", (c, e) => console.log("connectionError", e.message));
broker.on("publish", (p, c) => console.log("publish", p.topic, c?.id));

import mqtt from 'async-mqtt';

function delay(time) {
    return new Promise(resolve => setTimeout(resolve, time));
}

const sendMessages = async () => {

    const options = {
        port: 1883,
        username: 'mqtt-user',
        password: 'mqtt-user',
        clientId: 'mqttjs',
        clean: true,
    };

    const connection = await mqtt.connectAsync('mqtt://mqtt-user:mqtt-user@localhost:1883', options);

    console.log("Starting");
	try {
		await connection.publish("moves", '1');
		console.log("Done sending message: 1");
        await delay(1000);

        await connection.publish("moves", '0');
		console.log("Done sending message: 0");
        await delay(1000);
	} catch (e){
		// Do something about it!
		console.log(e.stack);
		process.exit();
	}
}

// Async function to connect to RabbitMQ and send a message
async function main() {
    while(true){
        await sendMessages();
    }
}

await main();
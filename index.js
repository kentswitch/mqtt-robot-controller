import mqtt from 'async-mqtt';
import enq from 'enquirer';

const sendMessages = async () => {

    const options = {
        port: 1883,
        username: 'mqtt-user',
        password: 'mqtt-user',
        clientId: 'mqttjs',
        clean: true,
    };

    const connection = await mqtt.connectAsync('mqtt://guest:guest@localhost:1883', options);

    /*
    // Subscribe to the topic 'feedback'
    connection.subscribe('feedback');

    // listen for messages
    connection.on('message', function (topic, message) {
        console.log(`\nMessage recived ${message.toString()} from topic ${topic.toString()}`);
    });
    */

    try {
        let question = new enq.Input({
            name: 'move',
            message: 'Robotun hangi yöne gitmesini istiyorsunuz?'
        });

        let move = await question.run();

        await connection.publish("moves", move);
    } catch (e) {
        // Do something about it!
        console.log(e.stack);
        process.exit();
    }
}

// Async function to connect to RabbitMQ and send a message
async function main() {
    while (true) {
        await sendMessages();
    }
}

await main();
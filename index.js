import { connect } from 'amqplib';


// Async function to connect to RabbitMQ and send a message
async function main() {
    // Set up connection and queue
    const queue = 'moves';
    const connection = await connect('amqp://guest:guest@localhost:5672');

    // Create a channel and queue
    const channel = await connection.createChannel();
    await channel.assertQueue(queue);

    const message = 'Arzuhal';
    // Send a message in given intervals
    setInterval(() => {
        channel.sendToQueue(queue, Buffer.from(message));
        console.log(`Sent ${message}`);
    }, 2500);
}

await main();
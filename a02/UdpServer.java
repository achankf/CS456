import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.Date;
import java.util.Deque;
import java.util.LinkedList;

/**
 * Most private helpers are self-explanatory. The only non-trivial parts are the
 * send/receiveHelpers.
 */

public final class UdpServer {

	private InetAddress IPAddress;
	private final File targetFile;
	private final int portEmu;
	private final DatagramSocket udpSocket;

	public UdpServer(String hostEmu, int portEmu, int portAck, File targetFile)
			throws IOException {

		this.portEmu = portEmu;
		this.targetFile = targetFile;
		udpSocket = new DatagramSocket(portAck);
		IPAddress = InetAddress.getByName(hostEmu);
		udpSocket.setSoTimeout(Constants.TIMEOUT);
	}

	private void sendSinglePacket(Packet packet) throws IOException {
		byte[] sendData = packet.getUDPdata();
		DatagramPacket sendPacket = new DatagramPacket(sendData,
				sendData.length, IPAddress, portEmu);
		udpSocket.send(sendPacket);
	}

	private Packet recievePacket() throws IOException {
		byte[] receiveData = new byte[1024];
		DatagramPacket receivePacket = new DatagramPacket(receiveData,
				receiveData.length);
		udpSocket.receive(receivePacket);
		return Packet.parseUDPdata(receiveData);
	}

	private String makeChunk(InputStream is) throws IOException {
		StringBuilder builder = new StringBuilder();
		int read;

		while (true) {
			read = is.read();
			if (read == -1) {
				break;
			}
			builder.append((char) read);
			if (builder.length() == Constants.MAX_DATA_LENGTH) {
				break;
			}
		}
		return builder.toString();
	}

	private Packet makePacket(int seqNum, InputStream is) throws IOException {
		String chunk = makeChunk(is);
		if (chunk.isEmpty()) {
			return null;
		}

		return Packet.createPacket(seqNum, chunk);
	}

	private void sendAllPackets(Iterable<Packet> packets) throws IOException {
		for (Packet packet : packets) {
			sendSinglePacket(packet);
		}
	}

	private void popPackets(Deque<Packet> packets, int k) {
		for (int i = 0; i < k; i++) {
			packets.pollFirst();
		}
	}

	private void sendHelper(InputStream is) throws IOException {
		int seqNum = 0, base = 0;
		LinkedList<Packet> packets = new LinkedList<Packet>();

		// push at most 10 packets to the queue at the beginning
		for (int i = 0; i < Constants.WINDOW_SIZE; i++) {
			Packet chunk = makePacket(seqNum, is);
			if (chunk == null) {
				break;
			}
			packets.add(chunk);
			seqNum++;
		}

		sendAllPackets(packets);

		Date start = new Date();

		while (!packets.isEmpty()) {
			int ack;
			Date now = new Date();

			// handle timeout
			if (now.getTime() - start.getTime() > Constants.TIMEOUT) {
				sendAllPackets(packets);
				start = now;
			}

			try {
				ack = recievePacket().getSeqNum();
			} catch (IOException ioe) {
				// timeout or other error
				continue;
			}

			// Get the number of packets being ACKed
			int numAcked = ack - base;

			// ``smooth" out the negative (wrapping) cases
			if (ack < base) {
				numAcked += Constants.SEQNUM_MODULO;
			}

			// drop duplicate ACKs
			if (numAcked == 0 || numAcked > Constants.WINDOW_SIZE) {
				continue;
			}

			popPackets(packets, numAcked);

			// push new packets into the queue
			for (int i = 0; i < numAcked; i++) {
				Packet nextPacket = makePacket(seqNum, is);
				if (nextPacket == null) {
					break;
				}
				packets.add(nextPacket);
				sendSinglePacket(nextPacket);
				seqNum++;
			}

			base = (base + numAcked) % Constants.SEQNUM_MODULO;
		}

		// send EOT at the end
		sendSinglePacket(Packet.createEOT(base));
	}

	// Sets up the InputStream and then call its helper for sending packets
	public void send() throws IOException {
		InputStream is = new BufferedInputStream(
				new FileInputStream(targetFile));
		try {
			sendHelper(is);
		} finally {
			is.close();
		}
	}

	private void receiveHelper(PrintStream ps) throws IOException {
		int expected = 0;

		boolean firstTime = true;

		RECEIVER_LOOP: while (true) {
			Packet packet;

			try {
				packet = recievePacket();
			} catch (IOException e) {
				// timeout and other errors
				continue;
			}

			if (packet.getSeqNum() != expected % Constants.SEQNUM_MODULO) {
				/*
				 * it is possible that the first packet is lost and we got the
				 * second packet here for the first time -- thus do not respond
				 * and let the sender to be timeout, and then resend the first
				 * packet.
				 */
				if (!firstTime) {
					sendSinglePacket(Packet.createACK(expected));
				}
				continue;
			}

			switch (packet.getType()) {
			case Constants.ACK:
				throw new IOException(
						"Something is VERY wrong: receiver should get any ACK");
			case Constants.DATA:
				ps.print(new String(packet.getData()));
				sendSinglePacket(Packet.createACK(expected));
				expected++;
				break;
			case Constants.EOT:
				break RECEIVER_LOOP;
			default:
				throw new IOException("No such packet type");
			}
			firstTime = false;
		}
	}

	// Sets up the PrintStream and then call its helper for receiving packets
	public void receive() throws IOException {
		PrintStream ps = new PrintStream(new FileOutputStream(targetFile));

		try {
			receiveHelper(ps);
		} finally {
			ps.close();
		}
	}
}

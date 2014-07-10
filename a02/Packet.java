// common Packet class used by both SENDER and RECEIVER

import java.io.IOException;
import java.nio.ByteBuffer;

public class Packet {

	// data members
	private int type;
	private int seqnum;
	private String data;

	/* CONSTRUCTORS */

	// hidden constructor to prevent creation of invalid Packets
	private Packet(int Type, int SeqNum, String strData) throws IOException {
		// if data seqment larger than allowed, then throw exception
		if (strData.length() > Constants.MAX_DATA_LENGTH)
			throw new IOException("data too large (max 500 chars)");

		type = Type;
		seqnum = SeqNum % Constants.SEQNUM_MODULO;
		data = strData;
	}

	// special Packet constructors to be used in place of hidden constructor
	public static Packet createACK(int SeqNum) throws IOException {
		return new Packet(Constants.ACK, SeqNum, new String());
	}

	public static Packet createPacket(int SeqNum, String data) throws IOException {
		return new Packet(Constants.DATA, SeqNum, data);
	}

	public static Packet createEOT(int SeqNum) throws IOException {
		return new Packet(Constants.EOT, SeqNum, new String());
	}

	/* PACKET DATA */

	public int getType() {
		return type;
	}

	public int getSeqNum() {
		return seqnum;
	}

	public int getLength() {
		return data.length();
	}

	public byte[] getData() {
		return data.getBytes();
	}

	/* UDP HELPERS */

	public byte[] getUDPdata() {
		ByteBuffer buffer = ByteBuffer.allocate(512);
		buffer.putInt(type);
		buffer.putInt(seqnum);
		buffer.putInt(data.length());
		buffer.put(data.getBytes(), 0, data.length());
		return buffer.array();
	}

	public static Packet parseUDPdata(byte[] UDPdata) throws IOException {
		ByteBuffer buffer = ByteBuffer.wrap(UDPdata);
		int type = buffer.getInt();
		int seqnum = buffer.getInt();
		int length = buffer.getInt();
		byte data[] = new byte[length];
		buffer.get(data, 0, length);
		return new Packet(type, seqnum, new String(data));
	}
}

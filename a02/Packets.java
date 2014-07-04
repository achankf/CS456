
import java.io.InputStream;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.Collection;

public class Packets {
	public static Collection<Packet> makePackets(InputStream is) throws IOException {
		int read;
		InputStream bis = new BufferedInputStream(is);
		StringBuilder builder = new StringBuilder();
		
		while ((read = bis.read()) != -1) {
			builder.append((char) read);
		}
		return null;
	}
}


import java.io.InputStream;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.FileNotFoundException;

public class Sender {

	public static void send(String hostEmu, int portEmu, int portAck, InputStream targetFile) throws Exception {
	}

	public static String fileToStr(String file) throws IOException{
		int read;
		InputStream bis = new BufferedInputStream(new FileInputStream(file));
		StringBuilder builder = new StringBuilder();
		
		while ((read = bis.read()) != -1) {
			builder.append((char) read);
		}
		return builder.toString();
	}

	public static void main(String[] args) throws Exception {
		String hostEmu;
		InputStream targetFile;
		int portEmu, portAck;

		if (args.length != 4) {
			System.out.println("Need 4 args");
			System.exit(-1);
		}

		hostEmu = args[0];
		portEmu = Integer.parseInt(args[1]);
		portAck = Integer.parseInt(args[2]);
		targetFile = new FileInputStream(args[3]);

		try {
			send(hostEmu, portEmu, portAck, targetFile);
		} finally {
			targetFile.close();
		}
	}
}

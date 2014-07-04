import java.io.File;

public class Receiver {

	public static void main(String[] args) throws Exception {

		if (args.length != 4) {
			System.out.println("Need 4 args");
			System.exit(-1);
		}

		UdpServer receiver = new UdpServer(args[0], Integer.parseInt(args[1]),
				Integer.parseInt(args[2]), new File(args[3]));
		receiver.receive();
	}
}

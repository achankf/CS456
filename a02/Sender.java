import java.io.File;

public class Sender {

	public static void main(String[] args) throws Exception {

		if (args.length != 4) {
			System.out.println("Need 4 args");
			System.exit(-1);
		}

		UdpServer sender = new UdpServer(args[0], Integer.parseInt(args[1]),
				Integer.parseInt(args[2]), new File(args[3]));
		sender.send();
	}
}

import java.net.Socket;
import java.util.Scanner;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.IOException;

/*
** The first thread is main. It reads data from the keyboard.
** The second thread is client. It reads data from the server.
*/
public class Main {

    public static void main( String[] args ) {

        String host = "192.168.1.105";
        int port = 27010;
        try ( Socket clientSocket = new Socket( host, port ) ){                 // java open socket and close him

            /*Client clientRunnable = new Client();
            Thread clientThread = new Thread( clientRunnable );                 // create a thread to work with the server.

            clientThread.start(); */                                              // start the thread

            // Working with text using the keyboard

            Scanner scanner = new Scanner( System.in );                         // reading from the console
            String line = null;

            // clientSocket.getOutputStream() - get access to the stream, true - flush
            System.out.println( "Client start read from keyboard" );
            PrintWriter serverFd = new PrintWriter( clientSocket.getOutputStream(), true );     // make stream
            while( ( line = scanner.nextLine() ).equals( "/stop" ) == false ){
                System.out.println( "From keyboard: " + line );
                serverFd.println( line );
            }

        }
        catch (IOException exlusion ){
            System.out.println( "Connectiog error" );
            throw new RuntimeException();
        }
    }
}


import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Helper class for writing individual bits to a standard OutputStream.
 */
class BitOutputStream {
    private final OutputStream out;
    private int buffer;
    private int bitCount;

    public BitOutputStream(OutputStream out) {
        this.out = out;
        this.buffer = 0;
        this.bitCount = 0;
    }

    public void writeBit(int bit) throws IOException {
        // Add the new bit to the buffer
        buffer = (buffer << 1) | bit;
        bitCount++;

        // If the buffer is full (8 bits), write it to the output stream
        if (bitCount == 8) {
            out.write(buffer);
            buffer = 0;
            bitCount = 0;
        }
    }

    public void close() throws IOException {
        // Write any remaining bits, padding with zeros to make a full byte
        while (bitCount > 0 && bitCount < 8) {
            writeBit(0);
        }
        out.close();
    }
}

/**
 * Helper class for reading individual bits from a standard InputStream.
 */
class BitInputStream {
    private final InputStream in;
    private int buffer;
    private int bitCount;

    public BitInputStream(InputStream in) {
        this.in = in;
        this.buffer = 0;
        this.bitCount = 0; // No bits in buffer initially
    }

    public int readBit() throws IOException {
        // If the buffer is empty, read the next byte from the input stream
        if (bitCount == 0) {
            int nextByte = in.read();
            if (nextByte == -1) {
                return -1; // End of stream
            }
            buffer = nextByte;
            bitCount = 8;
        }

        // Extract the most significant bit from the buffer
        int bit = (buffer >> (bitCount - 1)) & 1;
        bitCount--;
        return bit;
    }

    public void close() throws IOException {
        in.close();
    }
}
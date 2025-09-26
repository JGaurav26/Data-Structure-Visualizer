import javax.swing.*;
import javax.swing.border.EmptyBorder;
import java.awt.*;
import java.io.*;
import java.nio.file.Files; // <--- THIS IS THE FIX
import java.util.HashMap;
import java.util.Map;
import java.util.PriorityQueue;

/**
 * A complete Huffman Coding application with a graphical user interface.
 * This single file includes the Huffman logic and the Swing-based GUI.
 *
 * Data Structures Used:
 * 1. PriorityQueue (Min-Heap): To build the Huffman tree efficiently.
 * 2. HuffmanNode (Binary Tree): To represent the code structure.
 * 3. HashMap: To store character frequencies and the generated Huffman codes.
 */
public class HuffmanGUI extends JFrame {

    private final JTextArea statusArea;
    private File selectedFile;

    // --- Main Huffman Logic ---

    /**
     * Represents a node in the Huffman Tree.
     * Implements Comparable to be used in the PriorityQueue.
     */
    private static class HuffmanNode implements Comparable<HuffmanNode> {
        char character;
        int frequency;
        HuffmanNode left;
        HuffmanNode right;

        // Constructor for leaf nodes
        public HuffmanNode(char character, int frequency) {
            this.character = character;
            this.frequency = frequency;
        }

        // Constructor for internal nodes
        public HuffmanNode(int frequency, HuffmanNode left, HuffmanNode right) {
            this.character = '\0'; // Internal nodes have no character
            this.frequency = frequency;
            this.left = left;
            this.right = right;
        }

        public boolean isLeaf() {
            return this.left == null && this.right == null;
        }

        @Override
        public int compareTo(HuffmanNode other) {
            return this.frequency - other.frequency;
        }
    }

    /**
     * Builds the frequency map from an input file's bytes.
     */
    private Map<Character, Integer> buildFrequencyMap(byte[] fileBytes) {
        Map<Character, Integer> freqMap = new HashMap<>();
        for (byte b : fileBytes) {
            char c = (char) (b & 0xFF); // Treat byte as an unsigned char
            freqMap.put(c, freqMap.getOrDefault(c, 0) + 1);
        }
        return freqMap;
    }

    /**
     * Builds the Huffman Tree using a PriorityQueue (Min-Heap).
     * This is the core of the algorithm.
     */
    private HuffmanNode buildHuffmanTree(Map<Character, Integer> freqMap) {
        // Create a priority queue to act as a min-heap
        PriorityQueue<HuffmanNode> pq = new PriorityQueue<>();

        // Add all characters as leaf nodes to the priority queue
        for (Map.Entry<Character, Integer> entry : freqMap.entrySet()) {
            pq.add(new HuffmanNode(entry.getKey(), entry.getValue()));
        }

        // Build the tree by merging the two lowest-frequency nodes
        while (pq.size() > 1) {
            HuffmanNode left = pq.poll();
            HuffmanNode right = pq.poll();
            HuffmanNode parent = new HuffmanNode(left.frequency + right.frequency, left, right);
            pq.add(parent);
        }

        return pq.poll(); // The remaining node is the root of the tree
    }

    /**
     * Generates the Huffman codes for each character by traversing the tree.
     */
    private Map<Character, String> generateCodes(HuffmanNode root) {
        Map<Character, String> codeMap = new HashMap<>();
        generateCodesRecursive(root, "", codeMap);
        return codeMap;
    }

    private void generateCodesRecursive(HuffmanNode node, String code, Map<Character, String> codeMap) {
        if (node == null) return;
        if (node.isLeaf()) {
            codeMap.put(node.character, code);
        }
        generateCodesRecursive(node.left, code + "0", codeMap);
        generateCodesRecursive(node.right, code + "1", codeMap);
    }

    /**
     * Compresses the input file and writes to the output file.
     */
    public void encode(File inputFile, File outputFile) {
        try {
            byte[] fileBytes = Files.readAllBytes(inputFile.toPath());
            if (fileBytes.length == 0) {
                statusArea.append("Input file is empty. Nothing to compress.\n");
                return;
            }

            // 1. Build frequency map and Huffman tree
            Map<Character, Integer> freqMap = buildFrequencyMap(fileBytes);
            HuffmanNode root = buildHuffmanTree(freqMap);
            Map<Character, String> codeMap = generateCodes(root);

            // 2. Write the header (frequency map) and compressed data
            try (ObjectOutputStream oos = new ObjectOutputStream(new FileOutputStream(outputFile))) {
                oos.writeObject(freqMap); // Write frequency map for decoding

                BitOutputStream bos = new BitOutputStream(oos);
                for (byte b : fileBytes) {
                    char c = (char) (b & 0xFF);
                    String code = codeMap.get(c);
                    for (char bit : code.toCharArray()) {
                        bos.writeBit(bit - '0');
                    }
                }
                bos.close(); // Important to flush the last byte
            }
            long inputSize = inputFile.length();
            long outputSize = outputFile.length();
            double ratio = (double) outputSize / inputSize * 100;

            statusArea.append("Encoding successful!\n");
            statusArea.append(String.format("Original Size: %d bytes\n", inputSize));
            statusArea.append(String.format("Compressed Size: %d bytes\n", outputSize));
            statusArea.append(String.format("Compression Ratio: %.2f%%\n", ratio));

        } catch (IOException e) {
            statusArea.append("Error during encoding: " + e.getMessage() + "\n");
        }
    }

    /**
     * Decompresses the input file and writes to the output file.
     */
    public void decode(File inputFile, File outputFile) {
        try (ObjectInputStream ois = new ObjectInputStream(new FileInputStream(inputFile))) {
            // 1. Read the header (frequency map) and rebuild the Huffman tree
            @SuppressWarnings("unchecked")
            Map<Character, Integer> freqMap = (Map<Character, Integer>) ois.readObject();
            HuffmanNode root = buildHuffmanTree(freqMap);
            HuffmanNode currentNode = root;

            // 2. Read the compressed bit stream and decode
            try (FileOutputStream fos = new FileOutputStream(outputFile)) {
                BitInputStream bis = new BitInputStream(ois);
                int bit;
                while ((bit = bis.readBit()) != -1) {
                    if (bit == 0) {
                        currentNode = currentNode.left;
                    } else {
                        currentNode = currentNode.right;
                    }

                    if (currentNode.isLeaf()) {
                        fos.write((byte) currentNode.character);
                        currentNode = root; // Reset to root for the next character
                    }
                }
            }
            statusArea.append("Decoding successful!\n");

        } catch (IOException | ClassNotFoundException e) {
            statusArea.append("Error during decoding: " + e.getMessage() + "\n");
        }
    }


    // --- GUI Setup ---

    public HuffmanGUI() {
        setTitle("Huffman File Compressor");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(600, 400);
        setLocationRelativeTo(null); // Center the window

        // Main panel with padding
        JPanel mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.setBorder(new EmptyBorder(10, 10, 10, 10));
        setContentPane(mainPanel);

        // Top panel for file selection
        JPanel topPanel = new JPanel(new BorderLayout(10, 0));
        JLabel fileLabel = new JLabel("No file selected");
        fileLabel.setFont(new Font("SansSerif", Font.ITALIC, 12));
        JButton selectButton = new JButton("Select File...");
        topPanel.add(selectButton, BorderLayout.WEST);
        topPanel.add(fileLabel, BorderLayout.CENTER);
        mainPanel.add(topPanel, BorderLayout.NORTH);

        // Center panel for status messages
        statusArea = new JTextArea("Welcome to the Huffman Compressor!\nSelect a file to begin.\n");
        statusArea.setEditable(false);
        statusArea.setFont(new Font("Monospaced", Font.PLAIN, 12));
        JScrollPane scrollPane = new JScrollPane(statusArea);
        mainPanel.add(scrollPane, BorderLayout.CENTER);

        // Bottom panel for action buttons
        JPanel bottomPanel = new JPanel(new FlowLayout(FlowLayout.CENTER));
        JButton encodeButton = new JButton("Encode");
        JButton decodeButton = new JButton("Decode");
        bottomPanel.add(encodeButton);
        bottomPanel.add(decodeButton);
        mainPanel.add(bottomPanel, BorderLayout.SOUTH);

        // --- Action Listeners ---

        selectButton.addActionListener(e -> {
            JFileChooser fileChooser = new JFileChooser();
            int result = fileChooser.showOpenDialog(this);
            if (result == JFileChooser.APPROVE_OPTION) {
                selectedFile = fileChooser.getSelectedFile();
                fileLabel.setText(selectedFile.getName());
                statusArea.append("Selected file: " + selectedFile.getAbsolutePath() + "\n");
            }
        });

        encodeButton.addActionListener(e -> {
            if (selectedFile == null) {
                statusArea.append("Error: Please select a file to encode first.\n");
                return;
            }
            JFileChooser fileChooser = new JFileChooser();
            fileChooser.setSelectedFile(new File(selectedFile.getName() + ".huf"));
            int result = fileChooser.showSaveDialog(this);
            if (result == JFileChooser.APPROVE_OPTION) {
                File outputFile = fileChooser.getSelectedFile();
                statusArea.append("Encoding...\n");
                encode(selectedFile, outputFile);
            }
        });

        decodeButton.addActionListener(e -> {
            if (selectedFile == null) {
                statusArea.append("Error: Please select a file to decode first.\n");
                return;
            }
            if (!selectedFile.getName().endsWith(".huf")) {
                 statusArea.append("Warning: Selected file is not a '.huf' file. Decoding might fail.\n");
            }
            JFileChooser fileChooser = new JFileChooser();
            fileChooser.setSelectedFile(new File(selectedFile.getName().replace(".huf", ".decoded.txt")));
            int result = fileChooser.showSaveDialog(this);
            if (result == JFileChooser.APPROVE_OPTION) {
                File outputFile = fileChooser.getSelectedFile();
                statusArea.append("Decoding...\n");
                decode(selectedFile, outputFile);
            }
        });
    }

    public static void main(String[] args) {
        // Set a modern look and feel
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception e) {
            e.printStackTrace();
        }
        // Run the GUI on the Event Dispatch Thread
        SwingUtilities.invokeLater(() -> new HuffmanGUI().setVisible(true));
    }
}
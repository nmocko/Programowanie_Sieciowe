# Hangman Game

This project is a TCP-based Hangman game written in C, utilizing TLV (Type-Length-Value) encoding for communication. The game consists of a client-server architecture where the server selects a word, and the client tries to guess it. Additionally, announcements during the game are broadcasted over multicast.

## Table of Contents
1. [Features](#features)
2. [Requirements](#requirements)
3. [Installation](#installation)
4. [Usage](#usage)
5. [Configuration](#configuration)
6. [Protocol](#protocol)
7. [Multicast](#multicast)
9. [License](#license)

## Features
- Server selects a random word for the game.
- Client guesses letters of the word.
- Communication between client and server via TCP.
- TLV encoding used for communication protocol.
- Game announcements are sent via multicast.

## Requirements
- GCC (GNU Compiler Collection)
- Make
- POSIX-compliant operating system (Linux, macOS)
- Network environment supporting TCP and multicast

## Installation
1. Clone the repository:
    ```sh
    git clone https://github.com/yourusername/hangman-game.git
    cd hangman-game
    ```

2. Compile the server and client:
    ```sh
    cd server
    make
    cd ../client
    make
    ```

## Usage
1. Start the server:
    ```sh
    cd server
    ./server
    ```

2. Start the client:
    ```sh
    cd client
    ./client
    ```

The client will connect to the server and the game will begin. Follow the on-screen instructions to guess the letters of the word.

## Configuration
- **Server**: The server can be configured by modifying code in `server` folder. 
- **Client**: The client can be configured by modifying code in `client` folder. 

## Protocol
The communication between client and server is based on a simple TLV (Type-Length-Value) encoding scheme:
- **Type**: Identifies the type of message.
- **Length**: Specifies the length of the value field.
- **Value**: Contains the actual data.

## Multicast
The game includes multicast support for sending announcements during the game. Multicast settings can be adjusted in the `server` and `client` files. Ensure your network supports multicast to use this feature.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

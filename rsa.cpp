#include <algorithm>
#include <charconv>
#include <chrono>
#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random/uniform_int_distribution.hpp>

using boost::multiprecision::cpp_int;

// Бинарное возведение в степень.
cpp_int binaryPower(cpp_int base, cpp_int exponent) {
    cpp_int result = 1;
    while (exponent != 0) {
        if (exponent & 1) {
            result *= base;
        }
        exponent >>= 1;
        base *= base;
    }
    return result;
}

// Возведение в степень по модулю.
cpp_int modularPower(cpp_int base, cpp_int exponent, const cpp_int& modulus) {
    cpp_int result = 1;
    while (exponent != 0) {
        if (exponent & 1) {
            result *= base;
            result %= modulus;
        }
        base = (base * base) % modulus;
        exponent >>= 1;
    }
    return result % modulus;
}

// Коэффициенты относятся к аргументам в том порядке, в котором они переданы.
cpp_int extendedEuclidean(const cpp_int& firstNumber, const cpp_int& secondNumber,
                          cpp_int& firstCoefficient, cpp_int& secondCoefficient) {
    if (firstNumber == 0) {
        firstCoefficient = 0;
        secondCoefficient = 1;
        return secondNumber;
    }
    cpp_int commonDivisor = extendedEuclidean(secondNumber % firstNumber, firstNumber,
                                               firstCoefficient, secondCoefficient);
    cpp_int nextSecondCoefficient = firstCoefficient;
    firstCoefficient = secondCoefficient - (secondNumber / firstNumber) * firstCoefficient;
    secondCoefficient = nextSecondCoefficient;
    return commonDivisor;
}

cpp_int getRandomNumber(const cpp_int& lowerBound, const cpp_int& upperBound) {
    thread_local std::random_device randomSource;
    boost::random::uniform_int_distribution<cpp_int> offset(0, upperBound - lowerBound);
    return lowerBound + offset(randomSource);
}

bool millerRabinRound(cpp_int currentExponent, const cpp_int& candidate) {
    cpp_int base = getRandomNumber(2, candidate - 2);
    cpp_int residue = modularPower(base, currentExponent, candidate);
    if (residue == 1 || residue == candidate - 1) {
        return true;
    }

    const cpp_int lastExponent = (candidate - 1) / 2;
    while (currentExponent < lastExponent) {
        residue = (residue * residue) % candidate;
        currentExponent *= 2;
        if (residue == 1) {
            return false;
        }
        if (residue == candidate - 1) {
            return true;
        }
    }
    return false;
}

bool isProbablyPrime(const cpp_int& candidate) {
    if (candidate <= 1) {
        return false;
    }
    if (candidate <= 3) {
        return true;
    }
    if (candidate % 2 == 0 || candidate % 3 == 0) {
        return false;
    }

    cpp_int oddPart = candidate - 1;
    while (oddPart % 2 == 0) {
        oddPart /= 2;
    }

    constexpr int roundCount = 64;
    for (int roundNumber = 0; roundNumber < roundCount; ++roundNumber) {
        if (!millerRabinRound(oddPart, candidate)) {
            return false;
        }
    }
    return true;
}

// Решето Эратосфена для простых чисел меньше limit.
std::vector<int> sieveOfEratosthenes(int limit) {
    std::vector<bool> markedPrime(limit, true);
    std::vector<int> primes;
    if (limit > 2) {
        primes.push_back(2);
    }
    for (int divisor = 3; divisor * divisor < limit; divisor += 2) {
        if (markedPrime[divisor]) {
            for (int multiple = divisor * divisor; multiple < limit; multiple += divisor) {
                markedPrime[multiple] = false;
            }
        }
    }
    for (int candidate = 3; candidate < limit; candidate += 2) {
        if (markedPrime[candidate]) {
            primes.push_back(candidate);
        }
    }
    return primes;
}

cpp_int getFilteredCandidate(const std::vector<int>& smallPrimes,
                             const cpp_int& lowerBound, const cpp_int& upperBound) {
    while (true) {
        cpp_int candidate = getRandomNumber(lowerBound, upperBound) | 1;
        bool hasSmallDivisor = false;
        for (int divisor : smallPrimes) {
            if (divisor * divisor > candidate) {
                break;
            }
            if (candidate % divisor == 0) {
                hasSmallDivisor = true;
                break;
            }
        }
        if (!hasSmallDivisor) {
            return candidate;
        }
    }
}

cpp_int getPrimeNumber(const std::vector<int>& smallPrimes, unsigned int bitLength) {
    const cpp_int lowerBound = binaryPower(2, bitLength - 1);
    const cpp_int upperBound = lowerBound * 2 - 1;
    while (true) {
        cpp_int primeCandidate = getFilteredCandidate(smallPrimes, lowerBound, upperBound);
        if (isProbablyPrime(primeCandidate)) {
            return primeCandidate;
        }
    }
}

bool readKeyBitLength(unsigned int& keyBitLength) {
    constexpr unsigned int defaultKeyBitLength = 2048;
    std::string input;
    while (true) {
        std::cout << "Key length in bits [" << defaultKeyBitLength << "]: ";
        if (!std::getline(std::cin, input)) {
            return false;
        }
        const auto firstCharacter = input.find_first_not_of(" \t\r");
        if (firstCharacter == std::string::npos) {
            keyBitLength = defaultKeyBitLength;
            return true;
        }
        const char* end = input.data() + input.find_last_not_of(" \t\r") + 1;
        auto conversion = std::from_chars(input.data() + firstCharacter, end, keyBitLength);
        if (conversion.ec == std::errc{} && conversion.ptr == end && keyBitLength >= 4) {
            return true;
        }
        std::cout << "Enter a whole number from 4 to "
                  << std::numeric_limits<unsigned int>::max() << ".\n";
    }
}

int main() {
    unsigned int keyBitLength;
    if (!readKeyBitLength(keyBitLength)) {
        return 0;
    }
    const unsigned int firstPrimeBitLength = keyBitLength / 2;
    // Для четырёхбитного модуля 15 нужны простые числа 3 и 5.
    const unsigned int secondPrimeBitLength = std::max(3u, keyBitLength - firstPrimeBitLength);
    const std::vector<int> smallPrimes = sieveOfEratosthenes(400);

    cpp_int firstPrime, secondPrime, modulus;
    auto startTime = std::chrono::steady_clock::now();
    do {
        std::thread firstThread([&]() {
            firstPrime = getPrimeNumber(smallPrimes, firstPrimeBitLength);
        });
        std::thread secondThread([&]() {
            secondPrime = getPrimeNumber(smallPrimes, secondPrimeBitLength);
        });
        firstThread.join();
        secondThread.join();
        modulus = firstPrime * secondPrime;
    } while (firstPrime == secondPrime || boost::multiprecision::msb(modulus) + 1 != keyBitLength);

    auto elapsedTime = std::chrono::steady_clock::now() - startTime;
    std::cout << "Prime generation time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count()
              << " milliseconds\n";

    cpp_int eulerTotient = (firstPrime - 1) * (secondPrime - 1);
    // Небольшая случайная открытая экспонента ускоряет шифрование.
    const cpp_int maximumPublicExponent =
        std::min<cpp_int>(eulerTotient - 1, std::numeric_limits<int>::max());
    cpp_int publicExponent, privateExponent, totientCoefficient;
    do {
        publicExponent = getRandomNumber(3, maximumPublicExponent);
    } while (extendedEuclidean(publicExponent, eulerTotient, privateExponent, totientCoefficient) != 1);
    if (privateExponent < 0) {
        privateExponent += eulerTotient;
    }

    std::cout << "First prime: " << firstPrime << "\nSecond prime: " << secondPrime
              << "\nEuler's totient: " << eulerTotient
              << "\nPublic key (public exponent, modulus): (" << publicExponent << ", " << modulus << ")"
              << "\nPrivate key (private exponent, modulus): (" << privateExponent << ", " << modulus << ")\n";

    std::string message;
    std::cout << "Enter your message: ";
    if (!std::getline(std::cin, message)) {
        return 0;
    }

    std::queue<cpp_int> encryptedBlocks;
    for (char character : message) {
        auto messageByte = static_cast<unsigned char>(character);
        if (messageByte >= modulus) {
            std::cerr << "Every message byte must be smaller than the modulus. "
                      << "A key of 9 bits or more accepts all byte values.\n";
            return 1;
        }
        encryptedBlocks.push(modularPower(messageByte, publicExponent, modulus));
    }

    std::queue<cpp_int> decryptedBytes;
    while (!encryptedBlocks.empty()) {
        decryptedBytes.push(modularPower(encryptedBlocks.front(), privateExponent, modulus));
        encryptedBlocks.pop();
    }
    std::string decryptedMessage;
    while (!decryptedBytes.empty()) {
        decryptedMessage.push_back(static_cast<char>(decryptedBytes.front().convert_to<unsigned char>()));
        decryptedBytes.pop();
    }
    std::cout << "Message after decryption: " << decryptedMessage << '\n';
    return 0;
}

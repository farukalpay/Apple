import Foundation

print("🛡️  Welcome to NotaryScan v0.1")

let args = CommandLine.arguments.dropFirst()

if args.count < 2 {
    print("❗ Usage: ./NotaryScan <v1.app> <v2.app> [...additional versions]")
    exit(1)
}

var versionPaths: [String] = []
var phiValues: [Double] = []

for path in args {
    print("\n📦 Scanning: \(path)")

    do {
        let binaryPath = try findMainBinary(inApp: path)
        print("📍 Found binary: \(binaryPath)")

        let entropies = try computeEntropyBlocks(binaryPath: binaryPath)
        let phi = computePhi(entropies: entropies)
        print("🔎 φ (mean entropy): \(String(format: "%.4f", phi))")

        versionPaths.append(path)
        phiValues.append(phi)

    } catch {
        print("❌ Error: \(error.localizedDescription)")
    }
}

do {
    try writeCSV(versions: versionPaths, phis: phiValues)
} catch {
    print("❌ CSV write failed: \(error.localizedDescription)")
}

// MARK: - Helpers

func findMainBinary(inApp appPath: String) throws -> String {
    let plistPath = "\(appPath)/Contents/Info.plist"
    guard let plist = NSDictionary(contentsOfFile: plistPath),
          let execName = plist["CFBundleExecutable"] as? String else {
        throw NSError(domain: "NotaryScan", code: 2,
                      userInfo: [NSLocalizedDescriptionKey: "Cannot read CFBundleExecutable from Info.plist"])
    }

    let binaryPath = "\(appPath)/Contents/MacOS/\(execName)"
    if FileManager.default.fileExists(atPath: binaryPath) {
        return binaryPath
    } else {
        throw NSError(domain: "NotaryScan", code: 404,
                      userInfo: [NSLocalizedDescriptionKey: "Main binary not found at expected path: \(binaryPath)"])
    }
}

func computeEntropyBlocks(binaryPath: String, blockSize: Int = 4096) throws -> [Double] {
    guard let fileHandle = FileHandle(forReadingAtPath: binaryPath) else {
        throw NSError(domain: "NotaryScan", code: 1,
                      userInfo: [NSLocalizedDescriptionKey: "Unable to open \(binaryPath)"])
    }
    defer { fileHandle.closeFile() }

    var entropies: [Double] = []

    while autoreleasepool(invoking: {
        let blockData = fileHandle.readData(ofLength: blockSize)
        guard !blockData.isEmpty else { return false }
        var freq = [Int](repeating: 0, count: 256)
        blockData.forEach { freq[Int($0)] += 1 }
        let blockLen = blockData.count

        var entropy: Double = 0.0
        for count in freq where count > 0 {
            let p = Double(count) / Double(blockLen)
            entropy -= p * log2(p)
        }

        entropies.append(entropy)
        return true
    }) {}

    return entropies
}

func computePhi(entropies: [Double]) -> Double {
    guard !entropies.isEmpty else { return 0.0 }
    return entropies.reduce(0.0, +) / Double(entropies.count)
}

func writeCSV(versions: [String], phis: [Double], outputPath: String = "phi_summary.csv") throws {
    guard versions.count == phis.count else {
        throw NSError(domain: "NotaryScan", code: 3,
                      userInfo: [NSLocalizedDescriptionKey: "Mismatch between version paths and φ list"])
    }

    var csv = "Version,MeanEntropy\n"
    for (index, path) in versions.enumerated() {
        let versionName = URL(fileURLWithPath: path).lastPathComponent
        let phiVal = String(format: "%.4f", phis[index])
        csv += "\(versionName),\(phiVal)\n"
    }

    try csv.write(toFile: outputPath, atomically: true, encoding: .utf8)
    print("📝 CSV written to \(outputPath)")
}

import Darwin
import Foundation

typealias Completion = @convention(block) (CFDictionary?) -> Void
typealias GetNowPlayingInfo = @convention(c) (DispatchQueue, @escaping Completion) -> Void

let framework = dlopen("/System/Library/PrivateFrameworks/MediaRemote.framework/MediaRemote", RTLD_NOW)
guard let framework else {
    print("MediaRemote not available")
    exit(1)
}

guard let symbol = dlsym(framework, "MRMediaRemoteGetNowPlayingInfo") else {
    print("MRMediaRemoteGetNowPlayingInfo not available")
    exit(1)
}

let getNowPlayingInfo = unsafeBitCast(symbol, to: GetNowPlayingInfo.self)
let semaphore = DispatchSemaphore(value: 0)

getNowPlayingInfo(.global()) { dictionary in
    if let info = dictionary as? [String: Any] {
        for key in info.keys.sorted() {
            print("\(key): \(info[key] ?? "")")
        }
    } else {
        print("No now playing info")
    }
    semaphore.signal()
}

_ = semaphore.wait(timeout: .now() + 3)

import cv2
import numpy as np
import sys
import subprocess

def convolute(input, convolution):
    with open(convolution, 'r') as f:
        size = int(f.readline())
        kernel = []
        for _ in range(size):
            row = list(map(int, f.readline().split()))
            kernel.append(row)

    kernel = np.array(kernel, dtype=np.int32)
    acc = kernel.sum()

    image = cv2.imread(input, cv2.IMREAD_UNCHANGED)

    result = cv2.filter2D(image, -1, kernel, borderType=cv2.BORDER_CONSTANT)

    result = np.clip(result, 0, 255).astype(np.uint8)

    cv2.imwrite("./test/ref.bmp", result)

def sequential():
    failed = []
    images = ["lake.bmp", "city.bmp", "coast.bmp", "earth.bmp", "small_sample.bmp", "medium_sample.bmp", "big_sample.bmp", "motorcycle.bmp"]
    convolutions = ["3x3/id.conv", "3x3/gaussian_like.conv", "3x3/3d.conv", "5x5/id.conv", "5x5/some.conv"]
    print("TEST of sequential implementation:")
    for i in images:
        print(f"{i}:")
        for j in convolutions:
            print(f"{j}:")
            subprocess.run(f"./build/sequential ./data/image/{i} ./test/result.bmp ./data/convolution/{j}", shell=True)
            convolute(f"./data/image/{i}", f"./data/convolution/{j}")
            proc = subprocess.run("cmp -i 54 ./test/ref.bmp ./test/result.bmp", capture_output=True, text=True, shell=True)
            if proc.returncode != 0:
                print("test FAILED:")
                print(proc.stdout)
                failed.append([f"sequential on {i} with {j}"])
            else:
                print("test PASSED")
    print("##################################################################################")
    return failed

def main():
    failed = []
    failed.extend(sequential())

    if failed != []:
        print("\nfailed tests:")
        print("\n".join(failed))
        sys.exit(1)

if __name__ == "__main__":
    main()

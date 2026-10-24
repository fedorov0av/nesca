#include "IPRandomizer.h"


IPRandomizer::IPRandomizer(std::vector<IPRangeHolder> ipRangeVec, int shuffleGap) : ipRangeVec(ipRangeVec)
{
	this->shuffleGap = shuffleGap;

	for (int i = 0; i < ipRangeVec.size(); ++i) {
		this->shuffleOffset.push_back(0);
	}
}


IPRandomizer::~IPRandomizer()
{
	this->ipRangeVec.clear();
	this->shuffleOffset.clear();
}

void IPRandomizer::shuffleRange() {
	for (int i = 0; i < this->ipRangeVec.size(); ++i) {
		IPRangeHolder ipRangeHolder = this->ipRangeVec[i];
		if (ipRangeHolder.ip1 + this->shuffleOffset[i] > ipRangeHolder.ip2) {
			continue;
		}

		unsigned int remaining = ipRangeHolder.ip2 - (ipRangeHolder.ip1 + this->shuffleOffset[i]) + 1;
		unsigned int offset = (remaining < static_cast<unsigned int>(this->shuffleGap) ? remaining : static_cast<unsigned int>(this->shuffleGap));

		for (unsigned int j = 0; j < offset; ++j) {
			this->shuffledRange.push_back(ipRangeHolder.ip1 + this->shuffleOffset[i] + j);
		}

		this->shuffleOffset[i] += offset;
	}
	std::random_shuffle(this->shuffledRange.begin(), this->shuffledRange.end());
}

unsigned int IPRandomizer::getNext() {
	if (this->shuffledRange.empty()) {
		shuffleRange();

		//If still empty then ip-range chunk is depleted.
		if (this->shuffledRange.empty()) {
			return 0;
		};
	};

	unsigned int ip = this->shuffledRange[0];
	this->shuffledRange.erase(this->shuffledRange.begin());
	return ip;
}

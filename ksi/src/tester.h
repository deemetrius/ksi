#pragma once
#include <iostream>

struct num {
	using id = signed long long int;
	id num_;
	num(id n = 0) : num_(n) {
		std::cout << "construct num(" << num_ << ')' << std::endl;
	}
	num(const num & cn) : num_(cn.num_) {
		std::cout << "copy num(" << num_ << ')' << std::endl;
	}
	num & operator = (const num & cn) {
		num_ = cn.num_;
		std::cout << "assign num(" << num_ << ')' << std::endl;
		return *this;
	}
	~num() {
		std::cout << "destruct num(" << num_ << ')' << std::endl;
	}
};

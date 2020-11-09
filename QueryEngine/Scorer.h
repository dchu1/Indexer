#pragma once
#include <string>
#include <vector>

class Scorer
{
public:
	virtual double score(const std::vector<double>& args) const = 0;
	double _N;
	double _davg;
};
class ScorerBM25 : public Scorer
{
public:
	double score(const std::vector<double>& args) const override;
	ScorerBM25() : _k1{1.2}, _b{0.75}
	{}
	/*ScorerBM25(double N, double davg, double k1, double b) : _N{ N }, _davg{ davg }, _k1{ k1 }, _b{ b }
	{
	}*/
private:
	double _k1;
	double _b;
};
class ScorerCosMeasure : public Scorer
{
public:
	double score(const std::vector<double>& args) const override;
	/*ScorerCosMeasure(double N, double davg) : _N{ N }, _davg{ davg }
	{
	}*/
};